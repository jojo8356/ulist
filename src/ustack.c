#include "ulist.h"
#include "ugc.h"
#include "ugc_alloc.h"

/* ============================================================
 * UStack — Pile LIFO (backed by UVec)
 * ============================================================ */

UStack *ustack_new(size_t elem_size)
{
    UStack *s = xmalloc(sizeof(UStack));
    s->vec = uvec_new(elem_size);
    s->gc_managed = 0;
    return s;
}

UStack *ustack_new_gc(size_t elem_size)
{
    ugc_auto_init();
    UStack *s = ugc_xcalloc(1, 1, sizeof(UStack));
    s->vec = uvec_new_gc(elem_size); /* interne tracké aussi */
    s->gc_managed = 1;
    return s;
}

void ustack_free(UStack *s)
{
    if (!s) return;
    if (s->gc_managed) {
        /* Réclamation immédiate — sinon le GC s'en chargerait */
        uvec_free(s->vec);
        ugc_free(s);
        return;
    }
    uvec_free(s->vec);
    free(s);
}

void ustack_push(UStack *s, const void *elem)
{
    uvec_add(s->vec, elem);
}

void *ustack_peek(const UStack *s)
{
    if (!s) return NULL;
    return uvec_last(s->vec);
}

int ustack_pop(UStack *s)
{
    if (!s) return 0;
    return uvec_pop(s->vec);
}

int ustack_pop_into(UStack *s, void *out)
{
    if (!s || uvec_empty(s->vec)) return 0;
    void *top = uvec_last(s->vec);
    memcpy(out, top, s->vec->elem_size);
    uvec_pop(s->vec);
    return 1;
}

int ustack_size(const UStack *s)
{
    if (!s) return 0;
    return uvec_size(s->vec);
}

int ustack_empty(const UStack *s)
{
    if (!s) return 1;
    return uvec_empty(s->vec);
}

void ustack_clear(UStack *s)
{
    if (!s) return;
    uvec_clear(s->vec);
}
