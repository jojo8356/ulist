/* ============================================================
 * UGC — backend Boehm-Demers-Weiser (libgc)
 *
 * Implémente l'API ugc.h au-dessus du GC conservateur éprouvé de
 * Hans Boehm (www.hboehm.info/gc/), utilisé par Mono, Guile, GCJ…
 *
 * Ce fichier est OPTIONNEL : le build par défaut reste le backend
 * interne (src/ugc.c), zéro-dépendance. Compilation :
 *     make test-boehm BOEHM_DIR=/chemin/vers/prefixe
 *   (attend $(BOEHM_DIR)/include/gc.h et $(BOEHM_DIR)/lib/libgc.a,
 *    libgc compilée avec -DENABLE_DISCLAIM pour gc_disclaim.h)
 *
 * Parité de sémantique avec le backend interne :
 *  - Pile scannée, registres vidés, variables locales = racines
 *    (conservateur, comme l'interne — c'est le modèle de Boehm).
 *  - ugc_malloc/calloc/realloc/free comptabilisent EXACTEMENT :
 *    chaque objet porte un en-tête de 16 octets (taille demandée,
 *    magic, génération) et est alloué dans un « kind » dédié dont
 *    la disclaim-proc décrémente les compteurs quand le moteur
 *    réclame l'objet. mark_from_all=0 : l'objet mourant ne
 *    protège PAS ses enfants un cycle de plus, donc chaînes et
 *    cycles entiers s'effondrent en une seule collecte — comme
 *    le backend interne (contrairement à GC_register_finalizer /
 *    GC_finalized_malloc qui n'en libèrent qu'un niveau par
 *    collecte et avertissent « Finalization cycle »).
 *  - ugc_collect() force une collecte complète même quand la
 *    collecte automatique est coupée (enable → gcollect → disable
 *    soldés : GC_disable est un compteur à imbrication dans libgc).
 *  - Seuil + collecte auto gérés ici (le moteur Boehm reste
 *    GC_disable() hors collectes explicites → comportement
 *    déterministe identique à l'interne).
 *
 * Différences assumées (le moteur décide, pas la façade) :
 *  - Pas de fenêtre « jeune » : un objet non référencé peut être
 *    réclamé DÈS la première collecte (l'interne le protège une
 *    collecte de plus). Un résidu conservateur peut aussi retenir
 *    un objet une ou deux collectes — normal pour ce modèle.
 *  - ugc_set_stack_scan() / ugc_set_stack_base() : no-op (Boehm
 *    détecte la pile nativement, toujours conservateur).
 *  - ugc_shutdown() ne peut pas désallouer le tas interne de
 *    libgc (pas d'API) : il purge au mieux. C'est pourquoi
 *    test-boehm n'est PAS compilé avec LSan/ASan — le tas libgc
 *    serait rapporté comme « leak » à la sortie du processus.
 * ============================================================ */

#include "ugc.h"

#include <gc.h>
#include <gc_mark.h>
#include <gc_disclaim.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UGC_BOEHM_MAGIC ((unsigned int)0xB0E1B6Cdu) /* « Boehm B6C » */

/* En-tête posé devant chaque bloc utilisateur (16 octets :
 * alignement max_align_t préservé puisque GC_malloc est aligné
 * aux mots doubles sur x86_64). */
typedef struct {
    size_t       size;  /* taille DEMANDÉE (ugc_bytes est exact) */
    unsigned int magic;
    unsigned int gen;   /* génération d'ugc_init : ignore le passé */
} UGCBoehmHdr;

static struct {
    int           active;
    unsigned int  gen;          /* ++ à chaque ugc_init */
    size_t        count;        /* objets vivants trackés via l'API */
    size_t        bytes;        /* octets demandés vivants */
    size_t        threshold;
    int           auto_collect;
    int           verbose;
    GC_word       gc_no_base;
    int           held_disabled; /* !=0 si NOUS détenons GC_disable() */
    void       **roots;         /* slots racines explicites (anti-doublon) */
    size_t        roots_count, roots_cap;
} g_gc;

static int g_kind; /* kind dédié (disclaim comptable) — durée processus */

/* GC_disable()/GC_enable() forment un COMPTEUR à imbrication dans
 * libgc : appeler disable deux fois rendrait ugc_collect() inefficace.
 * Ces helpers n'agissent que sur transition réelle. */
static void boehm_hold_disabled(void)
{
    if (!g_gc.held_disabled) {
        GC_disable();
        g_gc.held_disabled = 1;
    }
}

static void boehm_release_enabled(void)
{
    if (g_gc.held_disabled) {
        GC_enable();
        g_gc.held_disabled = 0;
    }
}

/* ------------------------------------------------------------
 * Disclaim comptable : appelée par libgc quand un objet de notre
 * kind est prêt à être réclamé (verrou d'allocation tenu — le
 * backend UGC est mono-thread, les compteurs simples suffisent).
 * Retour 0 : toujours réclamer (jamais de résurrection).
 * ------------------------------------------------------------ */

static int GC_CALLBACK ugc_boehm_disclaim(void *obj);

static int GC_CALLBACK ugc_boehm_disclaim(void *obj)
{
    UGCBoehmHdr *h = (UGCBoehmHdr *)obj;
    if (h->magic != UGC_BOEHM_MAGIC) /* fragment de free-list ou passé */
        return 0;
    if (h->gen == g_gc.gen) {
        if (g_gc.count > 0)
            g_gc.count--;
        if (g_gc.bytes >= h->size)
            g_gc.bytes -= h->size;
        else
            g_gc.bytes = 0;
    }
    h->magic = 0; /* l'objet part : invalidé */
    return 0;
}

/* Retourne l'en-tête si ptr provient bien de ugc_malloc & co,
 * NULL sinon (pointeur intérieur brut, malloc() classique…). */
static UGCBoehmHdr *boehm_hdr_of(void *ptr)
{
    UGCBoehmHdr *h;
    if (!ptr)
        return NULL;
    h = (UGCBoehmHdr *)GC_base(ptr); /* base du bloc libgc, ou NULL */
    if (!h || (void *)(h + 1) != ptr || h->magic != UGC_BOEHM_MAGIC)
        return NULL;
    return h;
}

static void boehm_oom(const char *what, size_t size)
{
    fprintf(stderr, "ugc(boehm): %s a échoué (%zu octets demandés)\n",
            what, size);
    exit(1);
}

/* ------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------ */

void ugc_init(void)
{
    if (g_gc.active)
        return;
    GC_INIT();
    if (!g_kind) {
        /* Kind dédié : objets scannés conservativement (comme les
         * blocs normaux) + disclaim comptable qui NE protège PAS le
         * graphe fils (mark_from_all=0) → chaînes/cycles réclamés
         * en une collecte. */
        g_kind = (int)GC_new_kind(GC_new_free_list(), GC_DS_LENGTH,
                                  1 /* taille dans le descripteur */,
                                  1 /* objets neufs mis à zéro */);
        GC_register_disclaim_proc(g_kind, ugc_boehm_disclaim,
                                  0 /* mark_from_all */);
    }
    GC_set_finalize_on_demand(1); /* finaliseurs : déclenchés par nous */
    boehm_release_enabled();      /* solde les éventuels disable passés */
    boehm_hold_disabled();        /* collecte uniquement via ugc_collect */
    g_gc.count       = 0;
    g_gc.bytes       = 0;
    g_gc.gen++;
    g_gc.gc_no_base  = GC_get_gc_no();
    g_gc.threshold   = UGC_DEFAULT_THRESHOLD;
    g_gc.auto_collect = 1;
    g_gc.active = 1;
    if (g_gc.verbose)
        fprintf(stderr, "[ugc] init (backend Boehm, code version 0x%lx)\n",
                (unsigned long)GC_get_version());
}

void ugc_shutdown(void)
{
    if (!g_gc.active)
        return;
    ugc_collect();            /* purge + disclaims avant de couper */
    /* Désenregistrer CHAQUE slot : sans cela, libgc continuerait de
     * scanner pour toujours ces adresses (souvent des variables de
     * pile mortes réutilisées ensuite — rétention fantôme !). */
    for (size_t i = 0; i < g_gc.roots_count; i++)
        GC_remove_roots(g_gc.roots[i], (char *)g_gc.roots[i] + sizeof(void *));
    free(g_gc.roots);
    g_gc.roots       = NULL;
    g_gc.roots_count = 0;
    g_gc.roots_cap   = 0;
    g_gc.count       = 0;
    g_gc.bytes       = 0;
    g_gc.active = 0;
    /* libgc garde son tas pour le processus : pas d'API de vidage
     * global (c'est un GC pensé « pour toute la durée du processus »). */
}

void ugc_auto_init(void)
{
    if (!g_gc.active)
        ugc_init();
}

int ugc_is_active(void)
{
    return g_gc.active;
}

const char *ugc_backend_name(void)
{
    return "boehm";
}

/* ------------------------------------------------------------
 * Collection
 * ------------------------------------------------------------ */

void ugc_collect(void)
{
    if (!g_gc.active)
        return;
    /* GC_gcollect() est ignoré si libgc est désactivé — on force :
     * enable → collecte complète → restore (transitions soldées). */
    boehm_release_enabled();
    GC_gcollect();
    boehm_hold_disabled();
    GC_invoke_finalizers(); /* disclaims/finaliseurs déclenchés ici */
    if (g_gc.verbose)
        fprintf(stderr, "[ugc] collecte : %zu objets, %zu octets, tas %zu\n",
                g_gc.count, g_gc.bytes, GC_get_heap_size());
}

size_t ugc_count(void)
{
    return g_gc.count;
}

size_t ugc_bytes(void)
{
    return g_gc.bytes;
}

unsigned long ugc_collections(void)
{
    return (unsigned long)(GC_get_gc_no() - g_gc.gc_no_base);
}

/* ------------------------------------------------------------
 * Racines explicites — tableau maison (dédup) + GC_add_roots
 * ------------------------------------------------------------ */

size_t ugc_root_count(void)
{
    return g_gc.roots_count;
}

void ugc_add_root(void **slot)
{
    ugc_auto_init();
    if (!slot)
        return;
    for (size_t i = 0; i < g_gc.roots_count; i++)
        if (g_gc.roots[i] == (void *)slot)
            return; /* idempotent */
    if (g_gc.roots_count == g_gc.roots_cap) {
        size_t nc = g_gc.roots_cap ? g_gc.roots_cap * 2 : 8;
        void  **nr = realloc(g_gc.roots, nc * sizeof *nr);
        if (!nr)
            boehm_oom("add_root (grow)", nc * sizeof *nr);
        g_gc.roots     = nr;
        g_gc.roots_cap = nc;
    }
    g_gc.roots[g_gc.roots_count++] = (void *)slot;
    GC_add_roots(slot, (char *)slot + sizeof(void *));
}

void ugc_remove_root(void **slot)
{
    if (!g_gc.active || !slot)
        return;
    for (size_t i = 0; i < g_gc.roots_count; i++) {
        if (g_gc.roots[i] == (void *)slot) {
            g_gc.roots[i] = g_gc.roots[--g_gc.roots_count];
            GC_remove_roots(slot, (char *)slot + sizeof(void *));
            return;
        }
    }
}

/* ------------------------------------------------------------
 * Configuration
 * ------------------------------------------------------------ */

void ugc_set_threshold(size_t bytes)
{
    ugc_auto_init();
    g_gc.threshold = bytes;
}

void ugc_set_auto(int enabled)
{
    ugc_auto_init();
    g_gc.auto_collect = !!enabled;
}

void ugc_set_stack_scan(int enabled)
{
    /* Boehm est TOUJOURS conservateur sur la pile : pas d'option. */
    (void)enabled;
}

void ugc_set_stack_base(void *addr)
{
    /* Boehm détecte le bas de pile nativement à GC_INIT(). */
    (void)addr;
}

/* ------------------------------------------------------------
 * Allocation
 * ------------------------------------------------------------ */

void *ugc_malloc(size_t size)
{
    UGCBoehmHdr *h;
    ugc_auto_init();
    if (g_gc.auto_collect && g_gc.threshold > 0
        && g_gc.bytes + size > g_gc.threshold)
        ugc_collect();
    h = (UGCBoehmHdr *)GC_generic_malloc(sizeof(UGCBoehmHdr)
                                         + (size ? size : 1), g_kind);
    if (!h) {
        ugc_collect(); /* dernière chance */
        h = (UGCBoehmHdr *)GC_generic_malloc(sizeof(UGCBoehmHdr)
                                             + (size ? size : 1), g_kind);
        if (!h)
            boehm_oom("malloc", size);
    }
    h->size  = size;
    h->magic = UGC_BOEHM_MAGIC;
    h->gen   = g_gc.gen;
    g_gc.count++;
    g_gc.bytes += size;
    if (g_gc.verbose >= 2)
        fprintf(stderr, "[ugc] malloc(%zu) -> %p\n", size, (void *)(h + 1));
    return (void *)(h + 1);
}

void *ugc_calloc(size_t count, size_t size)
{
    size_t total;
    if (size != 0 && count > (size_t)-1 / size)
        boehm_oom("calloc (overflow)", count);
    total = count * size;
    return memset(ugc_malloc(total), 0, total ? total : 1);
}

void *ugc_realloc(void *ptr, size_t size)
{
    UGCBoehmHdr *h, *nh;
    size_t       old;
    if (!ptr)
        return ugc_malloc(size);
    if (size == 0) {
        ugc_free(ptr);
        return NULL;
    }
    h = boehm_hdr_of(ptr);
    if (!h) /* non tracké : passthrough comme le backend interne */
        return realloc(ptr, size);
    old = h->size;
    /* Réallocation manuelle : GC_realloc perdrait notre kind (et donc
     * le disclaim comptable). On alloue un bloc neuf du bon kind,
     * on copie, puis on libère l'ancien sans disclaim (GC_free). */
    nh = (UGCBoehmHdr *)GC_generic_malloc(sizeof(UGCBoehmHdr) + size, g_kind);
    if (!nh) {
        ugc_collect();
        nh = (UGCBoehmHdr *)GC_generic_malloc(sizeof(UGCBoehmHdr) + size,
                                              g_kind);
        if (!nh)
            boehm_oom("realloc", size);
    }
    memcpy((void *)(nh + 1), ptr, old < size ? old : size);
    nh->size  = size;
    nh->magic = UGC_BOEHM_MAGIC;
    nh->gen   = g_gc.gen;
    /* solde comptable : l'ancien meurt par GC_free (sans disclaim) */
    if (h->gen == g_gc.gen) {
        if (g_gc.count > 0)
            g_gc.count--;           /* l'ancien */
        if (g_gc.bytes >= old)
            g_gc.bytes -= old;
        else
            g_gc.bytes = 0;
    }
    h->magic = 0;
    h->size  = 0;
    GC_free(h);
    g_gc.count++;                   /* le nouveau */
    g_gc.bytes += size;
    return (void *)(nh + 1);
}

void ugc_free(void *ptr)
{
    UGCBoehmHdr *h;
    ugc_auto_init();
    if (!ptr)
        return;
    h = boehm_hdr_of(ptr);
    if (!h) {
        free(ptr); /* non tracké : comportement free() classique */
        return;
    }
    /* GC_free libère SANS appeler le disclaim (contrat libgc) :
     * on décrémente ici, aucune double-soustraction possible. */
    if (h->gen == g_gc.gen) {
        if (g_gc.count > 0)
            g_gc.count--;
        if (g_gc.bytes >= h->size)
            g_gc.bytes -= h->size;
        else
            g_gc.bytes = 0;
    }
    h->magic = 0;
    h->size  = 0;
    GC_free(h);
}

/* ------------------------------------------------------------
 * Debug
 * ------------------------------------------------------------ */

void ugc_set_verbose(int level)
{
    ugc_auto_init();
    g_gc.verbose = level;
}

void ugc_dump(void)
{
    int a = ugc_is_active();
    fprintf(stderr, "=== UGC dump ===\n");
    fprintf(stderr, "  backend        : boehm (libgc code 0x%lx)\n",
            (unsigned long)GC_get_version());
    fprintf(stderr, "  actif          : %s\n", a ? "oui" : "non");
    if (!a)
        return;
    fprintf(stderr, "  objets trackés : %zu\n", g_gc.count);
    fprintf(stderr, "  octets trackés : %zu\n", g_gc.bytes);
    fprintf(stderr, "  racines        : %zu\n", g_gc.roots_count);
    fprintf(stderr, "  collectes      : %lu\n", ugc_collections());
    fprintf(stderr, "  seuil          : %zu\n", g_gc.threshold);
    fprintf(stderr, "  auto-collect   : %s\n", g_gc.auto_collect ? "on" : "off");
    fprintf(stderr, "  tas libgc      : %zu octets (dont %zu libres)\n",
            GC_get_heap_size(), GC_get_free_bytes());
}
