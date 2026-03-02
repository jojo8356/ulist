#include "ulist.h"
#include "ugc.h"

/* ============================================================
 * UDeque — File double (backed by UDList)
 * ============================================================ */

UDeque *udeque_new(size_t elem_size)
{
    UDeque *d = xmalloc(sizeof(UDeque));
    d->list = udlist_new(elem_size);
    d->gc_managed = 0;
    return d;
}

UDeque *udeque_new_gc(size_t elem_size)
{
    UDeque *d = udeque_new(elem_size);
    d->gc_managed = 1;
    return d;
}

void udeque_free(UDeque *d)
{
    if (!d) return;
    udlist_free(d->list);
    free(d);
}

void udeque_push_front(UDeque *d, const void *elem)
{
    udlist_push_front(d->list, elem);
}

void udeque_push_back(UDeque *d, const void *elem)
{
    udlist_push_back(d->list, elem);
}

void *udeque_front(const UDeque *d)
{
    if (!d) return NULL;
    return udlist_front(d->list);
}

void *udeque_back(const UDeque *d)
{
    if (!d) return NULL;
    return udlist_back(d->list);
}

int udeque_pop_front(UDeque *d)
{
    if (!d) return 0;
    return udlist_pop_front(d->list);
}

int udeque_pop_back(UDeque *d)
{
    if (!d) return 0;
    return udlist_pop_back(d->list);
}

int udeque_pop_front_into(UDeque *d, void *out)
{
    if (!d || udlist_empty(d->list)) return 0;
    void *front = udlist_front(d->list);
    memcpy(out, front, d->list->elem_size);
    udlist_pop_front(d->list);
    return 1;
}

int udeque_pop_back_into(UDeque *d, void *out)
{
    if (!d || udlist_empty(d->list)) return 0;
    void *back = udlist_back(d->list);
    memcpy(out, back, d->list->elem_size);
    udlist_pop_back(d->list);
    return 1;
}

int udeque_size(const UDeque *d)
{
    if (!d) return 0;
    return udlist_size(d->list);
}

int udeque_empty(const UDeque *d)
{
    if (!d) return 1;
    return udlist_empty(d->list);
}
