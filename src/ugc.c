#include "ugc.h"
#include <stdlib.h>
#include <stdio.h>

/* Phase 0: All GC functions are stubs */

static int gc_active = 0;

void ugc_init(void)
{
    gc_active = 1;
}

void ugc_shutdown(void)
{
    gc_active = 0;
}

void ugc_collect(void)
{
    /* Not yet implemented */
}

size_t ugc_count(void)
{
    return 0;
}

size_t ugc_bytes(void)
{
    return 0;
}

void ugc_set_threshold(size_t bytes)
{
    (void)bytes;
}

void ugc_set_auto(int enabled)
{
    (void)enabled;
}

void *ugc_malloc(size_t size)
{
    /* Phase 0: just forward to malloc */
    return malloc(size);
}

void *ugc_calloc(size_t count, size_t size)
{
    return calloc(count, size);
}

void *ugc_realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

void ugc_free(void *ptr)
{
    free(ptr);
}

void ugc_auto_init(void)
{
    if (!gc_active) {
        ugc_init();
    }
}

int ugc_is_active(void)
{
    return gc_active;
}

void ugc_dump(void)
{
    fprintf(stderr, "UGC: %zu objects, %zu bytes (stub)\n",
            ugc_count(), ugc_bytes());
}

void ugc_set_verbose(int level)
{
    (void)level;
}
