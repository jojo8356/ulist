#include "ulist.h"
#include "ugc.h"

/* ============================================================
 * UStrList — Liste de strings (copies automatiques)
 * ============================================================ */

#define USTRLIST_INITIAL_CAPACITY 8

static void ustrlist_grow(UStrList *list)
{
    list->capacity *= 2;
    list->data = xrealloc(list->data, (size_t)list->capacity * sizeof(char *));
}

UStrList *ustrlist_new(void)
{
    UStrList *list = xmalloc(sizeof(UStrList));
    list->data = xmalloc(USTRLIST_INITIAL_CAPACITY * sizeof(char *));
    list->size = 0;
    list->capacity = USTRLIST_INITIAL_CAPACITY;
    list->gc_managed = 0;
    return list;
}

UStrList *ustrlist_new_gc(void)
{
    UStrList *list = ustrlist_new();
    list->gc_managed = 1;
    return list;
}

void ustrlist_free(UStrList *list)
{
    if (!list) return;
    for (int i = 0; i < list->size; i++)
        free(list->data[i]);
    free(list->data);
    free(list);
}

void ustrlist_add(UStrList *list, const char *str)
{
    if (list->size >= list->capacity)
        ustrlist_grow(list);
    list->data[list->size] = xmalloc(strlen(str) + 1);
    strcpy(list->data[list->size], str);
    list->size++;
}

void ustrlist_insert(UStrList *list, int index, const char *str)
{
    if (!list || index < 0 || index > list->size) return;
    if (list->size >= list->capacity)
        ustrlist_grow(list);
    for (int i = list->size; i > index; i--)
        list->data[i] = list->data[i - 1];
    list->data[index] = xmalloc(strlen(str) + 1);
    strcpy(list->data[index], str);
    list->size++;
}

const char *ustrlist_get(const UStrList *list, int index)
{
    if (!list || index < 0 || index >= list->size) return NULL;
    return list->data[index];
}

int ustrlist_size(const UStrList *list)
{
    if (!list) return 0;
    return list->size;
}

int ustrlist_empty(const UStrList *list)
{
    if (!list) return 1;
    return list->size == 0;
}

int ustrlist_remove(UStrList *list, int index)
{
    if (!list || index < 0 || index >= list->size) return 0;
    free(list->data[index]);
    for (int i = index; i < list->size - 1; i++)
        list->data[i] = list->data[i + 1];
    list->size--;
    return 1;
}

void ustrlist_clear(UStrList *list)
{
    if (!list) return;
    for (int i = 0; i < list->size; i++)
        free(list->data[i]);
    list->size = 0;
}

int ustrlist_find(const UStrList *list, const char *str)
{
    if (!list || !str) return -1;
    for (int i = 0; i < list->size; i++) {
        if (strcmp(list->data[i], str) == 0)
            return i;
    }
    return -1;
}

int ustrlist_contains(const UStrList *list, const char *str)
{
    return ustrlist_find(list, str) >= 0;
}

static int ustrlist_default_cmp(const void *a, const void *b)
{
    const char *sa = *(const char *const *)a;
    const char *sb = *(const char *const *)b;
    return strcmp(sa, sb);
}

void ustrlist_sort(UStrList *list)
{
    if (!list || list->size <= 1) return;
    qsort(list->data, (size_t)list->size, sizeof(char *), ustrlist_default_cmp);
}

void ustrlist_sort_fn(UStrList *list, uvec_cmp_fn cmp)
{
    if (!list || list->size <= 1 || !cmp) return;
    qsort(list->data, (size_t)list->size, sizeof(char *), cmp);
}

char *ustrlist_join(const UStrList *list, const char *sep)
{
    if (!list || list->size == 0) {
        char *empty = xmalloc(1);
        empty[0] = '\0';
        return empty;
    }
    size_t sep_len = sep ? strlen(sep) : 0;
    size_t total = 0;
    for (int i = 0; i < list->size; i++) {
        total += strlen(list->data[i]);
        if (i > 0) total += sep_len;
    }
    char *result = xmalloc(total + 1);
    char *p = result;
    for (int i = 0; i < list->size; i++) {
        if (i > 0 && sep_len > 0) {
            memcpy(p, sep, sep_len);
            p += sep_len;
        }
        size_t len = strlen(list->data[i]);
        memcpy(p, list->data[i], len);
        p += len;
    }
    *p = '\0';
    return result;
}

UStrList *ustrlist_from_split(const char *str, const char *delim)
{
    if (!str || !delim) return ustrlist_new();
    UStrList *list = ustrlist_new();
    size_t delim_len = strlen(delim);
    if (delim_len == 0) {
        ustrlist_add(list, str);
        return list;
    }
    const char *start = str;
    const char *found;
    while ((found = strstr(start, delim)) != NULL) {
        size_t len = (size_t)(found - start);
        char *part = xmalloc(len + 1);
        memcpy(part, start, len);
        part[len] = '\0';
        ustrlist_add(list, part);
        free(part);
        start = found + delim_len;
    }
    ustrlist_add(list, start);
    return list;
}
