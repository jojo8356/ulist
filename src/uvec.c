#include "ulist.h"
#include "ugc.h"

/* ============================================================
 * UVec — Tableau dynamique (Vector / ArrayList)
 * ============================================================ */

static void uvec_grow(UVec *v)
{
    v->capacity *= 2;
    v->data = xrealloc(v->data, (size_t)v->capacity * v->elem_size);
}

/* --- Création / Destruction --- */

UVec *uvec_new(size_t elem_size)
{
    UVec *v = xmalloc(sizeof(UVec));
    v->data = xmalloc(UVEC_INITIAL_CAPACITY * elem_size);
    v->elem_size = elem_size;
    v->size = 0;
    v->capacity = UVEC_INITIAL_CAPACITY;
    v->gc_managed = 0;
    return v;
}

UVec *uvec_new_gc(size_t elem_size)
{
    UVec *v = uvec_new(elem_size);
    v->gc_managed = 1;
    return v;
}

void uvec_free(UVec *v)
{
    if (!v) return;
    free(v->data);
    free(v);
}

/* --- Ajout --- */

void uvec_add(UVec *v, const void *elem)
{
    if (v->size >= v->capacity)
        uvec_grow(v);
    memcpy((char *)v->data + (size_t)v->size * v->elem_size, elem, v->elem_size);
    v->size++;
}

void uvec_insert(UVec *v, int index, const void *elem)
{
    if (index < 0 || index > v->size) return;
    if (v->size >= v->capacity)
        uvec_grow(v);
    char *base = (char *)v->data + (size_t)index * v->elem_size;
    int count = v->size - index;
    if (count > 0)
        memmove(base + v->elem_size, base, (size_t)count * v->elem_size);
    memcpy(base, elem, v->elem_size);
    v->size++;
}

/* --- Accès --- */

void *uvec_get(const UVec *v, int index)
{
    if (!v || index < 0 || index >= v->size) return NULL;
    return (char *)v->data + (size_t)index * v->elem_size;
}

void *uvec_first(const UVec *v)
{
    return uvec_get(v, 0);
}

void *uvec_last(const UVec *v)
{
    if (!v || v->size == 0) return NULL;
    return uvec_get(v, v->size - 1);
}

int uvec_size(const UVec *v)
{
    if (!v) return 0;
    return v->size;
}

int uvec_empty(const UVec *v)
{
    if (!v) return 1;
    return v->size == 0;
}

/* --- Modification --- */

void uvec_set(UVec *v, int index, const void *elem)
{
    if (!v || index < 0 || index >= v->size) return;
    memcpy((char *)v->data + (size_t)index * v->elem_size, elem, v->elem_size);
}

/* --- Suppression --- */

int uvec_remove(UVec *v, int index)
{
    if (!v || index < 0 || index >= v->size) return 0;
    char *base = (char *)v->data + (size_t)index * v->elem_size;
    int count = v->size - index - 1;
    if (count > 0)
        memmove(base, base + v->elem_size, (size_t)count * v->elem_size);
    v->size--;
    return 1;
}

int uvec_remove_fast(UVec *v, int index)
{
    if (!v || index < 0 || index >= v->size) return 0;
    if (index != v->size - 1) {
        char *target = (char *)v->data + (size_t)index * v->elem_size;
        char *last = (char *)v->data + (size_t)(v->size - 1) * v->elem_size;
        memcpy(target, last, v->elem_size);
    }
    v->size--;
    return 1;
}

int uvec_pop(UVec *v)
{
    if (!v || v->size == 0) return 0;
    v->size--;
    return 1;
}

void uvec_clear(UVec *v)
{
    if (!v) return;
    v->size = 0;
}

/* --- Recherche --- */

int uvec_find(const UVec *v, const void *elem)
{
    if (!v || !elem) return -1;
    for (int i = 0; i < v->size; i++) {
        if (memcmp((char *)v->data + (size_t)i * v->elem_size, elem, v->elem_size) == 0)
            return i;
    }
    return -1;
}

int uvec_find_fn(const UVec *v, uvec_predicate_fn pred, void *ctx)
{
    if (!v || !pred) return -1;
    for (int i = 0; i < v->size; i++) {
        if (pred((char *)v->data + (size_t)i * v->elem_size, ctx))
            return i;
    }
    return -1;
}

int uvec_contains(const UVec *v, const void *elem)
{
    return uvec_find(v, elem) >= 0;
}

/* --- Tri --- */

void uvec_sort(UVec *v, uvec_cmp_fn cmp)
{
    if (!v || v->size <= 1 || !cmp) return;
    qsort(v->data, (size_t)v->size, v->elem_size, cmp);
}

/* --- Built-in comparators --- */

int uvec_cmp_int(const void *a, const void *b)
{
    int va = *(const int *)a;
    int vb = *(const int *)b;
    return (va > vb) - (va < vb);
}

int uvec_cmp_int_desc(const void *a, const void *b)
{
    int va = *(const int *)a;
    int vb = *(const int *)b;
    return (vb > va) - (vb < va);
}

int uvec_cmp_double(const void *a, const void *b)
{
    double va = *(const double *)a;
    double vb = *(const double *)b;
    return (va > vb) - (va < vb);
}

int uvec_cmp_str(const void *a, const void *b)
{
    const char *sa = *(const char *const *)a;
    const char *sb = *(const char *const *)b;
    return strcmp(sa, sb);
}

/* --- Autres --- */

void uvec_reverse(UVec *v)
{
    if (!v || v->size <= 1) return;
    char tmp[256];
    char *use_tmp = tmp;
    char *heap_tmp = NULL;
    if (v->elem_size > sizeof(tmp)) {
        heap_tmp = xmalloc(v->elem_size);
        use_tmp = heap_tmp;
    }
    int lo = 0, hi = v->size - 1;
    while (lo < hi) {
        char *a = (char *)v->data + (size_t)lo * v->elem_size;
        char *b = (char *)v->data + (size_t)hi * v->elem_size;
        memcpy(use_tmp, a, v->elem_size);
        memcpy(a, b, v->elem_size);
        memcpy(b, use_tmp, v->elem_size);
        lo++;
        hi--;
    }
    free(heap_tmp);
}

UVec *uvec_clone(const UVec *v)
{
    if (!v) return NULL;
    UVec *c = xmalloc(sizeof(UVec));
    c->elem_size = v->elem_size;
    c->size = v->size;
    c->capacity = v->capacity;
    c->gc_managed = 0;
    c->data = xmalloc((size_t)c->capacity * c->elem_size);
    if (c->size > 0)
        memcpy(c->data, v->data, (size_t)c->size * c->elem_size);
    return c;
}

/* --- Fonctionnel --- */

void uvec_each(const UVec *v, uvec_each_fn fn, void *ctx)
{
    if (!v || !fn) return;
    for (int i = 0; i < v->size; i++)
        fn((char *)v->data + (size_t)i * v->elem_size, i, ctx);
}

UVec *uvec_filter(const UVec *v, uvec_predicate_fn pred, void *ctx)
{
    if (!v || !pred) return NULL;
    UVec *out = uvec_new(v->elem_size);
    for (int i = 0; i < v->size; i++) {
        void *elem = (char *)v->data + (size_t)i * v->elem_size;
        if (pred(elem, ctx))
            uvec_add(out, elem);
    }
    return out;
}

UVec *uvec_map(const UVec *v, size_t out_elem_size, uvec_map_fn fn, void *ctx)
{
    if (!v || !fn) return NULL;
    UVec *out = uvec_new(out_elem_size);
    char tmp[256];
    char *use_tmp = tmp;
    char *heap_tmp = NULL;
    if (out_elem_size > sizeof(tmp)) {
        heap_tmp = xmalloc(out_elem_size);
        use_tmp = heap_tmp;
    }
    for (int i = 0; i < v->size; i++) {
        fn((char *)v->data + (size_t)i * v->elem_size, use_tmp, ctx);
        uvec_add(out, use_tmp);
    }
    free(heap_tmp);
    return out;
}

void uvec_reduce(const UVec *v, uvec_reduce_fn fn, void *accumulator)
{
    if (!v || !fn || !accumulator) return;
    for (int i = 0; i < v->size; i++)
        fn((char *)v->data + (size_t)i * v->elem_size, accumulator);
}
