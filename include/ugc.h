#ifndef UGC_H
#define UGC_H

#include <stddef.h>

/* ============================================================
 * UGC — Garbage Collector (Mark-and-Sweep, conservateur)
 *
 * Comme les grands GC (Java, Go, Boehm GC) : une variable qui
 * n'est PLUS utilisée est libérée AUTOMATIQUEMENT, sans free().
 *
 * Racines AUTOMATIQUES — la pile est scannée :
 *  - À chaque collecte, les registres sont vidés sur la pile
 *    (setjmp) puis la pile est scannée mot par mot : toute
 *    VARIABLE LOCALE pointant vers un objet tracké (même via un
 *    pointeur intérieur) le garde en vie, ainsi que tout son
 *    graphe (scan conservateur des blocs atteints).
 *  - Quand la variable sort de son scope ou est réaffectée, la
 *    référence disparaît → l'objet est réclamé au prochain
 *    ugc_collect() (manuel, ou automatique via le seuil).
 *  - Les VARIABLES GLOBALES / statiques ne sont PAS scannées :
 *    utilisez ugc_add_root(&global) pour les ancrer.
 *
 * Objets trackés : toute allocation via ugc_malloc/ugc_calloc/
 * ugc_realloc, et toutes les structures créées via les
 * constructeurs *_new_gc() (leurs allocations internes sont
 * trackées automatiquement).
 *
 * Jeunes objets : un objet fraîchement alloué est protégé
 * jusqu'à la fin de la prochaine collecte — le temps qu'une
 * variable locale le référence.
 *
 * Collecte automatique : ugc_malloc déclenche ugc_collect()
 * quand le total alloué dépasse ugc_set_threshold() (si
 * ugc_set_auto(1)). Le seuil est adaptatif : il monte tout seul
 * si une collecte ne récupère presque rien (tas vivant) pour
 * éviter le thrashing.
 *
 * free() reste possible et IMMÉDIAT : uvec_free() & co libèrent
 * tout de suite, que la structure soit GC ou non. ugc_free()
 * sur un pointeur non tracké se comporte comme free().
 *
 * Exemple — zéro free() :
 *     void work(void) {
 *         UVec *v = uvec_new_gc(sizeof(int));
 *         for (int i = 0; i < 1000; i++)
 *             uvec_add_val(v, i);      // v vit sur la pile
 *     }                                // v n'est plus utilisé
 *     // ... au prochain ugc_collect() : tout est libéré
 *
 * Règles d'usage :
 *  - Appelez ugc_init() le plus tôt possible dans main() (sinon
 *    auto-init au premier usage — le haut de pile est alors
 *    détecté par la plateforme, avec repli heuristique).
 *  - Ne stockez des pointeurs trackés QUE dans des blocs trackés,
 *    des variables locales ou des racines explicites — jamais
 *    dans des blocs malloc() classiques (non scannés).
 *  - ugc_realloc() peut déplacer le bloc (comme realloc) : ne
 *    gardez pas de pointeurs intérieurs à travers un realloc.
 * ============================================================ */

#define UGC_DEFAULT_THRESHOLD ((size_t)1024 * 1024)  /* 1 MiB */

/* Lifecycle */
void   ugc_init(void);      /* idempotent ; appeler tôt dans main() */
void   ugc_shutdown(void);  /* libère TOUT ce qui est tracké + racines */

/* Collection */
void   ugc_collect(void);        /* mark-and-sweep complet */
size_t ugc_count(void);          /* nb d'objets trackés */
size_t ugc_bytes(void);          /* nb d'octets trackés */
size_t ugc_root_count(void);     /* nb de racines explicites */
unsigned long ugc_collections(void); /* nb de collectes depuis init */

/* Racines explicites (pour variables globalES/statiques ;
 * les variables locales sont détectées automatiquement) */
void   ugc_add_root(void **slot);
void   ugc_remove_root(void **slot);

/* Configuration */
void   ugc_set_threshold(size_t bytes); /* 0 = pas de collecte auto sur seuil */
void   ugc_set_auto(int enabled);       /* collecte auto dans ugc_malloc */
void   ugc_set_stack_scan(int enabled); /* scan de la pile (défaut : ON) */
void   ugc_set_stack_base(void *addr);  /* override du haut de pile */

/* Allocation (GC-tracked) — politique OOM : collecte + retry, puis exit(1)
 * comme les wrappers xmalloc/xcalloc/xrealloc de ulist.h */
void  *ugc_malloc(size_t size);
void  *ugc_calloc(size_t count, size_t size);
void  *ugc_realloc(void *ptr, size_t size);
void   ugc_free(void *ptr);

/* Auto-init (appelé par les variantes _new_gc et par les allocs) */
void        ugc_auto_init(void);
int         ugc_is_active(void);
/* Moteur actif : "internal" (défaut, zéro dépendance) ou
 * "boehm" (libgc Boehm-Demers-Weiser, voir make test-boehm) */
const char *ugc_backend_name(void);

/* Debug */
void   ugc_dump(void);        /* état du GC sur stderr */
void   ugc_set_verbose(int level); /* 0=silencieux, 1=collectes, 2=+allocs */

/* ============================================================
 * Header-only mode (préparé, pas encore actif)
 * ============================================================ */

#ifdef UGC_IMPLEMENTATION
/* Will contain inline implementations in a future phase */
#endif

#endif /* UGC_H */
