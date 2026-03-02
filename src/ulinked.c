#include "ulist.h"
#include "ugc.h"

/* ============================================================
 * ULinked — Liste chaînée simple
 * ============================================================ */

ULinked *ulinked_new(size_t elem_size)
{
    ULinked *list = xmalloc(sizeof(ULinked));
    list->head = NULL;
    list->elem_size = elem_size;
    list->size = 0;
    list->gc_managed = 0;
    return list;
}

ULinked *ulinked_new_gc(size_t elem_size)
{
    ULinked *list = ulinked_new(elem_size);
    list->gc_managed = 1;
    return list;
}

void ulinked_free(ULinked *list)
{
    if (!list) return;
    ULinkedNode *cur = list->head;
    while (cur) {
        ULinkedNode *next = cur->next;
        free(cur->data);
        free(cur);
        cur = next;
    }
    free(list);
}

static ULinkedNode *ulinked_make_node(const ULinked *list, const void *elem)
{
    ULinkedNode *node = xmalloc(sizeof(ULinkedNode));
    node->data = xmalloc(list->elem_size);
    memcpy(node->data, elem, list->elem_size);
    node->next = NULL;
    return node;
}

void ulinked_push(ULinked *list, const void *elem)
{
    ULinkedNode *node = ulinked_make_node(list, elem);
    node->next = list->head;
    list->head = node;
    list->size++;
}

void ulinked_append(ULinked *list, const void *elem)
{
    ULinkedNode *node = ulinked_make_node(list, elem);
    if (!list->head) {
        list->head = node;
    } else {
        ULinkedNode *cur = list->head;
        while (cur->next)
            cur = cur->next;
        cur->next = node;
    }
    list->size++;
}

void *ulinked_head(const ULinked *list)
{
    if (!list || !list->head) return NULL;
    return list->head->data;
}

int ulinked_size(const ULinked *list)
{
    if (!list) return 0;
    return list->size;
}

int ulinked_empty(const ULinked *list)
{
    if (!list) return 1;
    return list->size == 0;
}

int ulinked_pop(ULinked *list)
{
    if (!list || !list->head) return 0;
    ULinkedNode *old = list->head;
    list->head = old->next;
    free(old->data);
    free(old);
    list->size--;
    return 1;
}

int ulinked_remove(ULinked *list, int index)
{
    if (!list || index < 0 || index >= list->size) return 0;
    if (index == 0)
        return ulinked_pop(list);
    ULinkedNode *prev = list->head;
    for (int i = 0; i < index - 1; i++)
        prev = prev->next;
    ULinkedNode *target = prev->next;
    prev->next = target->next;
    free(target->data);
    free(target);
    list->size--;
    return 1;
}

void ulinked_clear(ULinked *list)
{
    if (!list) return;
    ULinkedNode *cur = list->head;
    while (cur) {
        ULinkedNode *next = cur->next;
        free(cur->data);
        free(cur);
        cur = next;
    }
    list->head = NULL;
    list->size = 0;
}

int ulinked_find(const ULinked *list, const void *elem)
{
    if (!list || !elem) return -1;
    ULinkedNode *cur = list->head;
    int i = 0;
    while (cur) {
        if (memcmp(cur->data, elem, list->elem_size) == 0)
            return i;
        cur = cur->next;
        i++;
    }
    return -1;
}

/* Merge sort for singly linked list */
static ULinkedNode *ulinked_merge(ULinkedNode *a, ULinkedNode *b, uvec_cmp_fn cmp)
{
    ULinkedNode dummy;
    dummy.next = NULL;
    ULinkedNode *tail = &dummy;
    while (a && b) {
        if (cmp(a->data, b->data) <= 0) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
    }
    tail->next = a ? a : b;
    return dummy.next;
}

static ULinkedNode *ulinked_merge_sort(ULinkedNode *head, uvec_cmp_fn cmp)
{
    if (!head || !head->next)
        return head;
    /* Split in half using slow/fast pointers */
    ULinkedNode *slow = head;
    ULinkedNode *fast = head->next;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    ULinkedNode *mid = slow->next;
    slow->next = NULL;
    ULinkedNode *left = ulinked_merge_sort(head, cmp);
    ULinkedNode *right = ulinked_merge_sort(mid, cmp);
    return ulinked_merge(left, right, cmp);
}

void ulinked_sort(ULinked *list, uvec_cmp_fn cmp)
{
    if (!list || list->size <= 1 || !cmp) return;
    list->head = ulinked_merge_sort(list->head, cmp);
}

void ulinked_reverse(ULinked *list)
{
    if (!list || list->size <= 1) return;
    ULinkedNode *prev = NULL;
    ULinkedNode *cur = list->head;
    while (cur) {
        ULinkedNode *next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    list->head = prev;
}
