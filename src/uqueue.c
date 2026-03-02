#include "ulist.h"
#include "ugc.h"

/* ============================================================
 * UQueue — File FIFO (backed by UDList)
 * ============================================================ */

UQueue *uqueue_new(size_t elem_size)
{
    UQueue *q = xmalloc(sizeof(UQueue));
    q->list = udlist_new(elem_size);
    q->gc_managed = 0;
    return q;
}

UQueue *uqueue_new_gc(size_t elem_size)
{
    UQueue *q = uqueue_new(elem_size);
    q->gc_managed = 1;
    return q;
}

void uqueue_free(UQueue *q)
{
    if (!q) return;
    udlist_free(q->list);
    free(q);
}

void uqueue_enqueue(UQueue *q, const void *elem)
{
    udlist_push_back(q->list, elem);
}

void *uqueue_peek(const UQueue *q)
{
    if (!q) return NULL;
    return udlist_front(q->list);
}

int uqueue_dequeue(UQueue *q)
{
    if (!q) return 0;
    return udlist_pop_front(q->list);
}

int uqueue_dequeue_into(UQueue *q, void *out)
{
    if (!q || udlist_empty(q->list)) return 0;
    void *front = udlist_front(q->list);
    memcpy(out, front, q->list->elem_size);
    udlist_pop_front(q->list);
    return 1;
}

int uqueue_size(const UQueue *q)
{
    if (!q) return 0;
    return udlist_size(q->list);
}

int uqueue_empty(const UQueue *q)
{
    if (!q) return 1;
    return udlist_empty(q->list);
}

void uqueue_clear(UQueue *q)
{
    if (!q) return;
    udlist_clear(q->list);
}
