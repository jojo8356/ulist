#ifndef UGC_ALLOC_H
#define UGC_ALLOC_H

/* ============================================================
 * ugc_alloc — helpers internes (non publics)
 *
 * Router les allocations internes des structures vers le GC
 * quand la structure est gc_managed, sinon vers les wrappers
 * classiques xmalloc/xcalloc/xrealloc/free.
 * ============================================================ */

#include "ulist.h"
#include "ugc.h"

static inline void *ugc_xmalloc(int gc, size_t size)
{
    return gc ? ugc_malloc(size) : xmalloc(size);
}

static inline void *ugc_xcalloc(int gc, size_t count, size_t size)
{
    return gc ? ugc_calloc(count, size) : xcalloc(count, size);
}

static inline void *ugc_xrealloc(int gc, void *ptr, size_t size)
{
    return gc ? ugc_realloc(ptr, size) : xrealloc(ptr, size);
}

static inline void ugc_xfree(int gc, void *ptr)
{
    if (gc) {
        ugc_free(ptr);
    } else {
        free(ptr);
    }
}

#endif /* UGC_ALLOC_H */
