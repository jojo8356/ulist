#include "ulist.h"
#include "ugc.h"

/* ============================================================
 * UDList — Liste chaînée double
 * ============================================================ */

UDList *udlist_new(size_t elem_size)
{
    UDList *list = xmalloc(sizeof(UDList));
    list->head = NULL;
    list->tail = NULL;
    list->elem_size = elem_size;
    list->size = 0;
    list->gc_managed = 0;
    return list;
}

UDList *udlist_new_gc(size_t elem_size)
{
    UDList *list = udlist_new(elem_size);
    list->gc_managed = 1;
    return list;
}

void udlist_free(UDList *list)
{
    if (!list) return;
    UDListNode *cur = list->head;
    while (cur) {
        UDListNode *next = cur->next;
        free(cur->data);
        free(cur);
        cur = next;
    }
    free(list);
}

static UDListNode *udlist_make_node(const UDList *list, const void *elem)
{
    UDListNode *node = xmalloc(sizeof(UDListNode));
    node->data = xmalloc(list->elem_size);
    memcpy(node->data, elem, list->elem_size);
    node->next = NULL;
    node->prev = NULL;
    return node;
}

void udlist_push_front(UDList *list, const void *elem)
{
    UDListNode *node = udlist_make_node(list, elem);
    node->next = list->head;
    if (list->head)
        list->head->prev = node;
    list->head = node;
    if (!list->tail)
        list->tail = node;
    list->size++;
}

void udlist_push_back(UDList *list, const void *elem)
{
    UDListNode *node = udlist_make_node(list, elem);
    node->prev = list->tail;
    if (list->tail)
        list->tail->next = node;
    list->tail = node;
    if (!list->head)
        list->head = node;
    list->size++;
}

void udlist_insert(UDList *list, int index, const void *elem)
{
    if (!list || index < 0 || index > list->size) return;
    if (index == 0) { udlist_push_front(list, elem); return; }
    if (index == list->size) { udlist_push_back(list, elem); return; }

    UDListNode *cur = list->head;
    for (int i = 0; i < index; i++)
        cur = cur->next;

    UDListNode *node = udlist_make_node(list, elem);
    node->prev = cur->prev;
    node->next = cur;
    cur->prev->next = node;
    cur->prev = node;
    list->size++;
}

void *udlist_front(const UDList *list)
{
    if (!list || !list->head) return NULL;
    return list->head->data;
}

void *udlist_back(const UDList *list)
{
    if (!list || !list->tail) return NULL;
    return list->tail->data;
}

void *udlist_get(const UDList *list, int index)
{
    if (!list || index < 0 || index >= list->size) return NULL;
    UDListNode *cur;
    if (index < list->size / 2) {
        cur = list->head;
        for (int i = 0; i < index; i++)
            cur = cur->next;
    } else {
        cur = list->tail;
        for (int i = list->size - 1; i > index; i--)
            cur = cur->prev;
    }
    return cur->data;
}

int udlist_size(const UDList *list)
{
    if (!list) return 0;
    return list->size;
}

int udlist_empty(const UDList *list)
{
    if (!list) return 1;
    return list->size == 0;
}

int udlist_pop_front(UDList *list)
{
    if (!list || !list->head) return 0;
    UDListNode *old = list->head;
    list->head = old->next;
    if (list->head)
        list->head->prev = NULL;
    else
        list->tail = NULL;
    free(old->data);
    free(old);
    list->size--;
    return 1;
}

int udlist_pop_back(UDList *list)
{
    if (!list || !list->tail) return 0;
    UDListNode *old = list->tail;
    list->tail = old->prev;
    if (list->tail)
        list->tail->next = NULL;
    else
        list->head = NULL;
    free(old->data);
    free(old);
    list->size--;
    return 1;
}

int udlist_remove(UDList *list, int index)
{
    if (!list || index < 0 || index >= list->size) return 0;
    if (index == 0) return udlist_pop_front(list);
    if (index == list->size - 1) return udlist_pop_back(list);

    UDListNode *cur = list->head;
    for (int i = 0; i < index; i++)
        cur = cur->next;
    cur->prev->next = cur->next;
    cur->next->prev = cur->prev;
    free(cur->data);
    free(cur);
    list->size--;
    return 1;
}

void udlist_clear(UDList *list)
{
    if (!list) return;
    UDListNode *cur = list->head;
    while (cur) {
        UDListNode *next = cur->next;
        free(cur->data);
        free(cur);
        cur = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

int udlist_find(const UDList *list, const void *elem)
{
    if (!list || !elem) return -1;
    UDListNode *cur = list->head;
    int i = 0;
    while (cur) {
        if (memcmp(cur->data, elem, list->elem_size) == 0)
            return i;
        cur = cur->next;
        i++;
    }
    return -1;
}

/* Merge sort for doubly linked list */
static UDListNode *udlist_split(UDListNode *head)
{
    UDListNode *slow = head;
    UDListNode *fast = head->next;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    UDListNode *mid = slow->next;
    slow->next = NULL;
    if (mid) mid->prev = NULL;
    return mid;
}

static UDListNode *udlist_merge(UDListNode *a, UDListNode *b, uvec_cmp_fn cmp)
{
    UDListNode dummy;
    dummy.next = NULL;
    UDListNode *tail = &dummy;
    while (a && b) {
        if (cmp(a->data, b->data) <= 0) {
            tail->next = a;
            a->prev = tail;
            a = a->next;
        } else {
            tail->next = b;
            b->prev = tail;
            b = b->next;
        }
        tail = tail->next;
    }
    UDListNode *rem = a ? a : b;
    tail->next = rem;
    if (rem) rem->prev = tail;
    /* Fix: dummy.next->prev should be NULL (it's the real head) */
    if (dummy.next) dummy.next->prev = NULL;
    return dummy.next;
}

static UDListNode *udlist_merge_sort(UDListNode *head, uvec_cmp_fn cmp)
{
    if (!head || !head->next)
        return head;
    UDListNode *mid = udlist_split(head);
    UDListNode *left = udlist_merge_sort(head, cmp);
    UDListNode *right = udlist_merge_sort(mid, cmp);
    return udlist_merge(left, right, cmp);
}

void udlist_sort(UDList *list, uvec_cmp_fn cmp)
{
    if (!list || list->size <= 1 || !cmp) return;
    list->head = udlist_merge_sort(list->head, cmp);
    /* Fix tail pointer */
    UDListNode *cur = list->head;
    while (cur->next)
        cur = cur->next;
    list->tail = cur;
}

void udlist_reverse(UDList *list)
{
    if (!list || list->size <= 1) return;
    UDListNode *cur = list->head;
    while (cur) {
        UDListNode *tmp = cur->next;
        cur->next = cur->prev;
        cur->prev = tmp;
        cur = tmp;
    }
    UDListNode *tmp = list->head;
    list->head = list->tail;
    list->tail = tmp;
}
