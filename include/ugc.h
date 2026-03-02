#ifndef UGC_H
#define UGC_H

#include <stddef.h>

/* ============================================================
 * UGC — Garbage Collector (Mark-and-Sweep)
 * Opt-in per structure via _new_gc() variants
 * ============================================================ */

/* Lifecycle */
void   ugc_init(void);
void   ugc_shutdown(void);

/* Collection */
void   ugc_collect(void);
size_t ugc_count(void);
size_t ugc_bytes(void);

/* Configuration */
void   ugc_set_threshold(size_t bytes);
void   ugc_set_auto(int enabled);

/* Allocation (GC-tracked) */
void  *ugc_malloc(size_t size);
void  *ugc_calloc(size_t count, size_t size);
void  *ugc_realloc(void *ptr, size_t size);
void   ugc_free(void *ptr);

/* Auto-init (called by _new_gc variants) */
void   ugc_auto_init(void);
int    ugc_is_active(void);

/* Debug */
void   ugc_dump(void);
void   ugc_set_verbose(int level);

/* ============================================================
 * Header-only mode (préparé, pas encore actif)
 * ============================================================ */

#ifdef UGC_IMPLEMENTATION
/* Will contain inline implementations in a future phase */
#endif

#endif /* UGC_H */
