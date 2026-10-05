#include "ugc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>

#if defined(_WIN32)
#   ifndef _WIN32_WINNT
#       define _WIN32_WINNT 0x0602 /* GetCurrentThreadStackLimits (Win8+) */
#   endif
#   include <windows.h>
#elif defined(__APPLE__)
#   include <pthread.h>
#elif defined(__linux__)
/* glibc : haut de pile du thread principal ; symbole weak car absent
 * de certaines libc statiques (ex. musl) → repli heuristique */
extern void *__libc_stack_end __attribute__((weak));
#endif

/* ============================================================
 * UGC — Garbage Collector (Mark-and-Sweep, conservateur)
 *
 * Phase 2 : libération automatique dès qu'une variable n'est
 * plus utilisée — la pile fait office de racines (comme les GC
 * Boehm, Java, Go).
 *
 *  - Registre : liste chaînée de tous les blocs alloués via
 *    ugc_malloc / ugc_calloc / ugc_realloc.
 *  - Racines  : (1) la PILE du thread courant, scannée
 *    conservativement à chaque collecte (variables locales) ;
 *    (2) les racines explicites ugc_add_root() (variables
 *    globales).
 *  - Mark     : itératif (worklist explicite, pas de récursion)
 *    depuis les racines ; scan conservateur mot par mot du
 *    contenu des blocs atteints.
 *  - Sweep    : libère les blocs non marqués, démarque les
 *    autres.
 *
 * Notes :
 *  - Les objets jeunes sont marqués à leur création (protégés
 *    pour un cycle) mais ne sont scannés qu'une fois atteints
 *    depuis une racine — garantit la fenêtre d'ancrage dans une
 *    variable locale sans lire de blocs non initialisés.
 *  - Un registre peut contenir la seule référence vivante : on
 *    le vide sur la pile via setjmp avant de scanner (méthode
 *    éprouvée du GC Boehm).
 *  - Le scan conservateur peut garder un objet en vie par
 *    accident (valeur ressemblant à un pointeur, fantôme de
 *    stack) — rétention temporaire acceptée, jamais de
 *    libération prématurée d'un objet référencé.
 * ============================================================ */

typedef struct GCObj {
    void          *ptr;
    size_t         size;
    unsigned char  marked;   /* atteint pendant ce cycle */
    unsigned char  young;    /* alloué depuis la dernière collecte */
    struct GCObj  *next;
} GCObj;

typedef struct GCRoot {
    void         **slot;
    struct GCRoot *next;
} GCRoot;

static struct {
    GCObj         *objects;
    GCRoot        *roots;
    size_t         count;
    size_t         bytes;
    size_t         roots_count;
    size_t         threshold;
    unsigned long  collections;
    void          *stack_base;      /* haut de pile (fin du scan) */
    int            stack_base_user; /* base fournie par l'utilisateur */
    int            stack_scan;      /* scan de la pile actif */
    int            active;
    int            auto_collect;
    int            collecting;
    int            verbose;
} g_gc = {
    NULL, NULL,
    0, 0, 0,
    UGC_DEFAULT_THRESHOLD,
    0,
    NULL, 0, 1,
    0, 1, 0, 0
};

/* ------------------------------------------------------------
 * Helpers internes
 * ------------------------------------------------------------ */

static void ugc_oom(const char *what, size_t size)
{
    fprintf(stderr, "ugc: out of memory (%s %zu bytes)\n", what, size);
    exit(1);
}

/* Recherche par adresse exacte (bookkeeping free/realloc) */
static GCObj *gc_find_exact(const void *p)
{
    for (GCObj *o = g_gc.objects; o; o = o->next) {
        if (o->ptr == p)
            return o;
    }
    return NULL;
}

/* ------------------------------------------------------------
 * Snapshot trié des blocs pour le marking : recherche dichotomique
 * O(log n) au lieu d'un parcours linéaire par candidat — sinon le
 * scan conservateur est quadratique sur les gros tas.
 * ------------------------------------------------------------ */

typedef struct {
    uintptr_t lo;   /* adresse de début du bloc            */
    uintptr_t hi;   /* adresse de fin (exclue), == lo si 0 */
    GCObj    *obj;
} GCRange;

static int gc_range_cmp(const void *a, const void *b)
{
    uintptr_t la = ((const GCRange *)a)->lo;
    uintptr_t lb = ((const GCRange *)b)->lo;
    return (la > lb) - (la < lb);
}

static const GCRange *g_ranges = NULL;
static size_t         g_ranges_count = 0;

static GCRange *gc_snapshot_build(size_t *out_count)
{
    size_t n = 0;
    for (GCObj *o = g_gc.objects; o; o = o->next)
        n++;
    GCRange *rs = (GCRange *)malloc((n ? n : 1) * sizeof *rs);
    if (!rs)
        ugc_oom("mark snapshot", n * sizeof *rs);
    size_t i = 0;
    for (GCObj *o = g_gc.objects; o; o = o->next) {
        uintptr_t lo = (uintptr_t)o->ptr;
        rs[i].lo  = lo;
        rs[i].hi  = lo + (uintptr_t)o->size;
        rs[i].obj = o;
        i++;
    }
    qsort(rs, n, sizeof *rs, gc_range_cmp);
    *out_count = n;
    return rs;
}

/* Recherche inclusive : p à l'intérieur de [lo, hi) d'un bloc */
static GCObj *gc_snapshot_find(uintptr_t c)
{
    const GCRange *rs = g_ranges;
    size_t lo = 0, hi = g_ranges_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (rs[mid].lo <= c)
            lo = mid + 1;
        else
            hi = mid;
    }
    /* lo = nombre de plages commençant à ou avant c */
    if (lo == 0)
        return NULL;
    const GCRange *r = &rs[lo - 1];
    if (r->lo < r->hi && c < r->hi) /* hi == lo : bloc de taille 0 */
        return r->obj;
    return NULL;
}

static void gc_track(void *ptr, size_t size)
{
    GCObj *o = (GCObj *)malloc(sizeof *o);
    if (!o)
        ugc_oom("tracking", size);
    o->ptr    = ptr;
    o->size   = size;
    o->marked = 1;   /* né marqué : protégé jusqu'à la fin du prochain cycle */
    o->young  = 1;
    o->next   = g_gc.objects;
    g_gc.objects = o;
    g_gc.count++;
    g_gc.bytes += size;
    if (g_gc.verbose >= 2)
        fprintf(stderr, "[ugc] + %p (%zu bytes)\n", ptr, size);
}

/* Retire l'entrée du registre sans libérer le bloc utilisateur */
static GCObj *gc_detach(GCObj **link)
{
    GCObj *o = *link;
    *link = o->next;
    g_gc.count--;
    g_gc.bytes -= o->size;
    return o;
}

/* ------------------------------------------------------------
 * Worklist itérative pour le marking (pas de récursion →
 * chaînes profondes sans risque pour la pile)
 * ------------------------------------------------------------ */

typedef struct {
    GCObj **items;
    size_t  len;
    size_t  cap;
} GCWork;

static void gc_work_push(GCWork *w, GCObj *o)
{
    if (o->marked)
        return;
    o->marked = 1;
    if (w->len == w->cap) {
        size_t ncap = w->cap ? w->cap * 2 : 128;
        GCObj **ni = (GCObj **)realloc(w->items, ncap * sizeof *ni);
        if (!ni) {
            free(w->items);
            ugc_oom("mark worklist", ncap * sizeof *ni);
        }
        w->items = ni;
        w->cap   = ncap;
    }
    w->items[w->len++] = o;
}

/* Scan conservateur : chaque mot du bloc est testé comme pointeur
 * potentiel vers un bloc tracké (pointeurs intérieurs acceptés) */
static void gc_scan(GCWork *w, const GCObj *o)
{
    const unsigned char *base = (const unsigned char *)o->ptr;
    size_t words = o->size / sizeof(void *);
    for (size_t i = 0; i < words; i++) {
        void *candidate = NULL;
        memcpy(&candidate, base + i * sizeof(void *), sizeof candidate);
        if (!candidate)
            continue;
        GCObj *target = gc_snapshot_find((uintptr_t)candidate);
        if (target)
            gc_work_push(w, target);
    }
}

/* ------------------------------------------------------------
 * Racines automatiques : la pile du thread courant
 *
 * Détection du haut de pile par plateforme, avec repli sur la
 * frame de ugc_init() (d'où la recommandation d'appeler
 * ugc_init() tôt dans main(), ou ugc_set_stack_base()).
 * ------------------------------------------------------------ */

static void *gc_detect_stack_base(void)
{
#if defined(_WIN32)
    ULONG_PTR lo = 0, hi = 0;
    GetCurrentThreadStackLimits(&lo, &hi);
    return (void *)hi;              /* pile descendante : hi = base */
#elif defined(__APPLE__)
    return pthread_get_stackaddr_np(pthread_self());
#elif defined(__linux__)
    if (&__libc_stack_end != NULL)
        return (void *)__libc_stack_end;
    return NULL;
#else
    return NULL;
#endif
}

/* Vide les registres sur la pile (méthode Boehm) puis scanne
 * [sp, stack_base) mot par mot : toute variable locale pointant
 * vers un bloc tracké l'ancre. no_sanitize_address : la lecture
 * conservative de la pile brute dépasse volontairement les objets
 * C déclarés (c'est le principe même du scan). */
__attribute__((no_sanitize_address))
static void gc_mark_stack(GCWork *w)
{
    if (!g_gc.stack_scan || !g_gc.stack_base)
        return;

    jmp_buf regs;
    (void)setjmp(regs); /* claque tous les registres dans regs (sur la pile) */

    uintptr_t here = (uintptr_t)&regs;
    uintptr_t base = (uintptr_t)g_gc.stack_base;
    uintptr_t align = (uintptr_t)sizeof(void *) - 1;
    uintptr_t lo = (here + align) & ~align;
    uintptr_t hi = base & ~align;
    if (base <= here || lo >= hi)
        return; /* configuration inattendue : on ne scanne rien */

    for (uintptr_t p = lo; p < hi; p += sizeof(void *)) {
        void *candidate = NULL;
        memcpy(&candidate, (const unsigned char *)p, sizeof candidate);
        if (!candidate)
            continue;
        GCObj *o = gc_snapshot_find((uintptr_t)candidate);
        if (o && !o->marked) {
            if (g_gc.verbose >= 2)
                fprintf(stderr, "[ugc] stack: %p anchored by slot %p\n",
                        o->ptr, (void *)p);
            gc_work_push(w, o);
        }
    }
}

static void gc_mark(void)
{
    GCWork w = { NULL, 0, 0 };

    /* Graines racines explicites : valeur courante des slots */
    for (GCRoot *r = g_gc.roots; r; r = r->next) {
        if (!r->slot)
            continue;
        void *candidate = NULL;
        memcpy(&candidate, r->slot, sizeof candidate);
        if (!candidate)
            continue;
        GCObj *o = gc_snapshot_find((uintptr_t)candidate);
        if (o)
            gc_work_push(&w, o);
    }

    /* Graines racines automatiques : la pile (variables locales) */
    gc_mark_stack(&w);

    /* Propagation : les objets jeunes sont déjà marqués (naissent
     * marqués) mais ne sont scannés qu'une fois atteints depuis une
     * racine — on ne scanne que le graphe effectivement ancré. */
    while (w.len > 0) {
        GCObj *o = w.items[--w.len];
        gc_scan(&w, o);
    }
    free(w.items);
}

static size_t gc_sweep(size_t *freed_bytes_out)
{
    size_t freed = 0;
    size_t fbytes = 0;
    GCObj **link = &g_gc.objects;
    while (*link) {
        GCObj *o = *link;
        if (!o->marked) {
            gc_detach(link);
            if (g_gc.verbose >= 2)
                fprintf(stderr, "[ugc] - %p (%zu bytes)\n", o->ptr, o->size);
            fbytes += o->size;
            free(o->ptr);
            free(o);
            freed++;
        } else {
            o->marked = 0;
            o->young  = 0;
            link = &o->next;
        }
    }
    if (freed_bytes_out)
        *freed_bytes_out = fbytes;
    return freed;
}

/* ------------------------------------------------------------
 * Nettoyage des frames internes du GC
 *
 * La machinerie de collecte (snapshot, qsort, find) manipule des
 * adresses d'objets et en laisse des copies dans ses frames mortes.
 * Sans nettoyage, le scan de pile de la collecte SUIVANTE lirait
 * ces fantômes internes et garderait des objets en vie par erreur
 * (auto-rétention). Boehm GC fait la même chose (GC_clear_stack) :
 * on écrase toute la zone sous la frame courante entre les phases.
 * ------------------------------------------------------------ */

__attribute__((noinline))
static void gc_stack_scrub_below(void)
{
    volatile unsigned char dummy[256 * 1024];
    for (size_t i = 0; i < sizeof dummy; i++)
        dummy[i] = 0;
}

/* ------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------ */

void ugc_init(void)
{
    if (g_gc.active)
        return;
    g_gc.objects     = NULL;
    g_gc.roots       = NULL;
    g_gc.count       = 0;
    g_gc.bytes       = 0;
    g_gc.roots_count = 0;
    g_gc.collections = 0;
    g_gc.collecting  = 0;
    /* Haut de pile : détection plateforme, sinon la frame courante
     * (appeler ugc_init() tôt dans main() pour un scan maximal) */
    if (!g_gc.stack_base_user) {
        g_gc.stack_base = gc_detect_stack_base();
        if (!g_gc.stack_base) {
            int frame_marker;
            g_gc.stack_base = (void *)&frame_marker;
        }
    }
    g_gc.active      = 1;
}

void ugc_shutdown(void)
{
    if (!g_gc.active)
        return;
    /* Libère tous les blocs trackés, marqués ou non */
    while (g_gc.objects) {
        GCObj *o = g_gc.objects;
        g_gc.objects = o->next;
        free(o->ptr);
        free(o);
    }
    while (g_gc.roots) {
        GCRoot *r = g_gc.roots;
        g_gc.roots = r->next;
        free(r);
    }
    g_gc.count       = 0;
    g_gc.bytes       = 0;
    g_gc.roots_count = 0;
    g_gc.collections = 0;
    g_gc.collecting  = 0;
    g_gc.active      = 0;
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
    return "internal";
}

/* ------------------------------------------------------------
 * Collection
 * ------------------------------------------------------------ */

void ugc_collect(void)
{
    ugc_auto_init();
    if (g_gc.collecting)
        return;
    g_gc.collecting = 1;
    g_gc.collections++;
    /* Snapshot trié pour les recherches dichotomiques du marking */
    GCRange *snapshot = gc_snapshot_build(&g_ranges_count);
    g_ranges = snapshot;
    /* Détruit les fantômes d'adresses laissés sur la pile par
     * build/qsort (sinon auto-rétention à la prochaine collecte) */
    gc_stack_scrub_below();
    gc_mark();
    g_ranges = NULL;
    g_ranges_count = 0;
    free(snapshot);

    size_t bytes_before = g_gc.bytes;
    size_t freed_bytes = 0;
    size_t freed = gc_sweep(&freed_bytes);

    /* Seuil adaptatif anti-thrash : si la collecte n'a presque rien
     * récupéré (moins d'1/8e du tas) alors que le seuil est atteint,
     * c'est que le tas est majoritairement vivant — sans ajustement,
     * un seuil bas provoquerait une collecte quasiment à chaque
     * allocation. On double alors le seuil sur la taille vivante. */
    if (g_gc.threshold > 0
        && freed_bytes < bytes_before / 8
        && g_gc.bytes > g_gc.threshold / 2) {
        g_gc.threshold = g_gc.bytes * 2;
    }

    if (g_gc.verbose >= 1) {
        fprintf(stderr,
                "[ugc] collect #%lu: %zu freed, %zu live, %zu bytes\n",
                g_gc.collections, freed, g_gc.count, g_gc.bytes);
    }
    g_gc.collecting = 0;
}

size_t ugc_count(void)      { return g_gc.count; }
size_t ugc_bytes(void)      { return g_gc.bytes; }
size_t ugc_root_count(void) { return g_gc.roots_count; }
unsigned long ugc_collections(void) { return g_gc.collections; }

/* ------------------------------------------------------------
 * Racines
 * ------------------------------------------------------------ */

void ugc_add_root(void **slot)
{
    if (!slot)
        return;
    ugc_auto_init();
    for (GCRoot *r = g_gc.roots; r; r = r->next) {
        if (r->slot == slot)
            return; /* déjà enregistré : idempotent */
    }
    GCRoot *r = (GCRoot *)malloc(sizeof *r);
    if (!r)
        ugc_oom("root", sizeof *r);
    r->slot = slot;
    r->next = g_gc.roots;
    g_gc.roots = r;
    g_gc.roots_count++;
}

void ugc_remove_root(void **slot)
{
    if (!slot)
        return;
    GCRoot **link = &g_gc.roots;
    while (*link) {
        if ((*link)->slot == slot) {
            GCRoot *r = *link;
            *link = r->next;
            free(r);
            g_gc.roots_count--;
        } else {
            link = &(*link)->next;
        }
    }
}

/* ------------------------------------------------------------
 * Configuration
 * ------------------------------------------------------------ */

void ugc_set_threshold(size_t bytes)
{
    g_gc.threshold = bytes;
}

void ugc_set_auto(int enabled)
{
    g_gc.auto_collect = enabled ? 1 : 0;
}

void ugc_set_verbose(int level)
{
    if (level < 0) level = 0;
    if (level > 2) level = 2;
    g_gc.verbose = level;
}

void ugc_set_stack_scan(int enabled)
{
    g_gc.stack_scan = enabled ? 1 : 0;
}

void ugc_set_stack_base(void *addr)
{
    g_gc.stack_base = addr;
    g_gc.stack_base_user = 1;
}

/* ------------------------------------------------------------
 * Allocation trackée
 * ------------------------------------------------------------ */

void *ugc_malloc(size_t size)
{
    ugc_auto_init();
    if (g_gc.auto_collect && g_gc.threshold > 0 && !g_gc.collecting
        && g_gc.bytes + size > g_gc.threshold) {
        ugc_collect(); /* collecte avant d'allouer (seuil atteint) */
    }
    void *p = malloc(size ? size : 1);
    if (!p) {
        ugc_collect();       /* dernière chance : réclamer les déchets */
        p = malloc(size ? size : 1);
        if (!p)
            ugc_oom("malloc", size);
    }
    gc_track(p, size);
    return p;
}

void *ugc_calloc(size_t count, size_t size)
{
    if (size != 0 && count > SIZE_MAX / size)
        ugc_oom("calloc (overflow)", count);
    size_t total = count * size;
    void *p = ugc_malloc(total);
    memset(p, 0, total ? total : 1);
    return p;
}

void *ugc_realloc(void *ptr, size_t size)
{
    if (!ptr)
        return ugc_malloc(size);
    if (size == 0) {
        ugc_free(ptr);
        return NULL;
    }
    GCObj *o = gc_find_exact(ptr);
    if (!o) {
        /* Bloc non tracké : passthrough (compat Phase 0) */
        void *p = realloc(ptr, size);
        if (!p)
            ugc_oom("realloc", size);
        return p;
    }
    /* Bloc tracké : nouveau bloc intégralement initialisé (le scanner
     * conservateur peut le lire) puis copie du contenu existant. */
    void *p = malloc(size);
    if (!p) {
        ugc_collect();
        p = malloc(size);
        if (!p)
            ugc_oom("realloc", size);
    }
    memset(p, 0, size);
    memcpy(p, o->ptr, o->size < size ? o->size : size);
    free(o->ptr);
    g_gc.bytes = g_gc.bytes - o->size + size;
    o->ptr  = p;
    o->size = size;
    return p;
}

void ugc_free(void *ptr)
{
    if (!ptr)
        return;
    for (GCObj **link = &g_gc.objects; *link; link = &(*link)->next) {
        if ((*link)->ptr == ptr) {
            GCObj *o = gc_detach(link);
            if (g_gc.verbose >= 2)
                fprintf(stderr, "[ugc] - %p (%zu bytes)\n", o->ptr, o->size);
            free(o->ptr);
            free(o);
            return;
        }
    }
    /* Non tracké : se comporte comme free() (compat Phase 0) */
    free(ptr);
}

/* ------------------------------------------------------------
 * Debug
 * ------------------------------------------------------------ */

void ugc_dump(void)
{
    fprintf(stderr,
            "[ugc] objects=%zu bytes=%zu roots=%zu collections=%lu "
            "threshold=%zu auto=%d active=%d\n",
            g_gc.count, g_gc.bytes, g_gc.roots_count, g_gc.collections,
            g_gc.threshold, g_gc.auto_collect, g_gc.active);
}
