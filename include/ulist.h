#ifndef ULIST_H
#define ULIST_H

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>

/* ============================================================
 * UList — Universal List (C)
 * Structures de données génériques sans boilerplate
 * ============================================================ */

/* ------------------------------------------------------------
 * Safe allocation wrappers
 * ------------------------------------------------------------ */

static inline void *xmalloc(size_t size)
{
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, "ulist: out of memory (malloc %zu bytes)\n", size);
        exit(1);
    }
    return p;
}

static inline void *xcalloc(size_t count, size_t size)
{
    void *p = calloc(count, size);
    if (!p) {
        fprintf(stderr, "ulist: out of memory (calloc %zu x %zu bytes)\n", count, size);
        exit(1);
    }
    return p;
}

static inline void *xrealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);
    if (!p && size > 0) {
        fprintf(stderr, "ulist: out of memory (realloc %zu bytes)\n", size);
        exit(1);
    }
    return p;
}

/* ------------------------------------------------------------
 * Callback typedefs
 * ------------------------------------------------------------ */

typedef int  (*uvec_cmp_fn)(const void *a, const void *b);
typedef int  (*uvec_predicate_fn)(const void *elem, void *ctx);
typedef void (*uvec_each_fn)(const void *elem, int index, void *ctx);
typedef void (*uvec_map_fn)(const void *in, void *out, void *ctx);
typedef void (*uvec_reduce_fn)(const void *elem, void *accumulator);

/* ============================================================
 * UVec — Tableau dynamique (Vector / ArrayList)
 * ============================================================ */

#define UVEC_INITIAL_CAPACITY 8

typedef struct {
    void *data;
    size_t elem_size;
    int size;
    int capacity;
    int gc_managed;
} UVec;

/* Création / Destruction */
UVec *uvec_new(size_t elem_size);
UVec *uvec_new_gc(size_t elem_size);
void  uvec_free(UVec *v);

/* Ajout */
void  uvec_add(UVec *v, const void *elem);
void  uvec_insert(UVec *v, int index, const void *elem);

/* Accès */
void *uvec_get(const UVec *v, int index);
void *uvec_first(const UVec *v);
void *uvec_last(const UVec *v);
int   uvec_size(const UVec *v);
int   uvec_empty(const UVec *v);

/* Modification */
void  uvec_set(UVec *v, int index, const void *elem);

/* Suppression */
int   uvec_remove(UVec *v, int index);
int   uvec_remove_fast(UVec *v, int index);
int   uvec_pop(UVec *v);
void  uvec_clear(UVec *v);

/* Recherche */
int   uvec_find(const UVec *v, const void *elem);
int   uvec_find_fn(const UVec *v, uvec_predicate_fn pred, void *ctx);
int   uvec_contains(const UVec *v, const void *elem);

/* Tri */
void  uvec_sort(UVec *v, uvec_cmp_fn cmp);

/* Built-in comparators */
int   uvec_cmp_int(const void *a, const void *b);
int   uvec_cmp_int_desc(const void *a, const void *b);
int   uvec_cmp_double(const void *a, const void *b);
int   uvec_cmp_str(const void *a, const void *b);

/* Autres */
void  uvec_reverse(UVec *v);
UVec *uvec_clone(const UVec *v);

/* Fonctionnel */
void  uvec_each(const UVec *v, uvec_each_fn fn, void *ctx);
UVec *uvec_filter(const UVec *v, uvec_predicate_fn pred, void *ctx);
UVec *uvec_map(const UVec *v, size_t out_elem_size, uvec_map_fn fn, void *ctx);
void  uvec_reduce(const UVec *v, uvec_reduce_fn fn, void *accumulator);

/* ============================================================
 * UVec Macros
 * ============================================================ */

#define uvec_add_val(v, val) \
    do { __typeof__(val) _uv_tmp = (val); uvec_add((v), &_uv_tmp); } while(0)

#define uvec_insert_val(v, idx, val) \
    do { __typeof__(val) _uv_tmp = (val); uvec_insert((v), (idx), &_uv_tmp); } while(0)

#define uvec_set_val(v, idx, val) \
    do { __typeof__(val) _uv_tmp = (val); uvec_set((v), (idx), &_uv_tmp); } while(0)

#define uvec_get_as(v, idx, type) \
    (*(type *)uvec_get((v), (idx)))

#define uvec_get_typed(v, idx, type) \
    ((type *)uvec_get((v), (idx)))

#define uvec_find_val(v, val) \
    ({ __typeof__(val) _uv_tmp = (val); uvec_find((v), &_uv_tmp); })

#define uvec_contains_val(v, val) \
    ({ __typeof__(val) _uv_tmp = (val); uvec_contains((v), &_uv_tmp); })

#define uvec_foreach(v, type, var) \
    for (int _uv_i = 0; _uv_i < uvec_size(v); _uv_i++) \
        for (type *var = (type *)uvec_get((v), _uv_i); var; var = NULL)

/* ============================================================
 * ULinked — Liste chaînée simple
 * ============================================================ */

typedef struct ULinkedNode {
    void *data;
    struct ULinkedNode *next;
} ULinkedNode;

typedef struct {
    ULinkedNode *head;
    size_t elem_size;
    int size;
    int gc_managed;
} ULinked;

ULinked *ulinked_new(size_t elem_size);
ULinked *ulinked_new_gc(size_t elem_size);
void     ulinked_free(ULinked *list);

void     ulinked_push(ULinked *list, const void *elem);
void     ulinked_append(ULinked *list, const void *elem);
void    *ulinked_head(const ULinked *list);
int      ulinked_size(const ULinked *list);
int      ulinked_empty(const ULinked *list);
int      ulinked_pop(ULinked *list);
int      ulinked_remove(ULinked *list, int index);
void     ulinked_clear(ULinked *list);
int      ulinked_find(const ULinked *list, const void *elem);
void     ulinked_sort(ULinked *list, uvec_cmp_fn cmp);
void     ulinked_reverse(ULinked *list);

#define ulinked_push_val(list, val) \
    do { __typeof__(val) _ul_tmp = (val); ulinked_push((list), &_ul_tmp); } while(0)

#define ulinked_append_val(list, val) \
    do { __typeof__(val) _ul_tmp = (val); ulinked_append((list), &_ul_tmp); } while(0)

#define ulinked_head_as(list, type) \
    (*(type *)ulinked_head((list)))

#define ulinked_find_val(list, val) \
    ({ __typeof__(val) _ul_tmp = (val); ulinked_find((list), &_ul_tmp); })

#define ulinked_foreach(list, type, var) \
    for (ULinkedNode *_ul_n = (list) ? (list)->head : NULL; _ul_n; _ul_n = _ul_n->next) \
        for (type *var = (type *)_ul_n->data; var; var = NULL)

/* ============================================================
 * UDList — Liste chaînée double
 * ============================================================ */

typedef struct UDListNode {
    void *data;
    struct UDListNode *next;
    struct UDListNode *prev;
} UDListNode;

typedef struct {
    UDListNode *head;
    UDListNode *tail;
    size_t elem_size;
    int size;
    int gc_managed;
} UDList;

UDList  *udlist_new(size_t elem_size);
UDList  *udlist_new_gc(size_t elem_size);
void     udlist_free(UDList *list);

void     udlist_push_front(UDList *list, const void *elem);
void     udlist_push_back(UDList *list, const void *elem);
void     udlist_insert(UDList *list, int index, const void *elem);
void    *udlist_front(const UDList *list);
void    *udlist_back(const UDList *list);
void    *udlist_get(const UDList *list, int index);
int      udlist_size(const UDList *list);
int      udlist_empty(const UDList *list);
int      udlist_pop_front(UDList *list);
int      udlist_pop_back(UDList *list);
int      udlist_remove(UDList *list, int index);
void     udlist_clear(UDList *list);
int      udlist_find(const UDList *list, const void *elem);
void     udlist_sort(UDList *list, uvec_cmp_fn cmp);
void     udlist_reverse(UDList *list);

#define udlist_push_front_val(list, val) \
    do { __typeof__(val) _ud_tmp = (val); udlist_push_front((list), &_ud_tmp); } while(0)

#define udlist_push_back_val(list, val) \
    do { __typeof__(val) _ud_tmp = (val); udlist_push_back((list), &_ud_tmp); } while(0)

#define udlist_front_as(list, type) (*(type *)udlist_front((list)))
#define udlist_back_as(list, type)  (*(type *)udlist_back((list)))

#define udlist_foreach(list, type, var) \
    for (UDListNode *_ud_n = (list) ? (list)->head : NULL; _ud_n; _ud_n = _ud_n->next) \
        for (type *var = (type *)_ud_n->data; var; var = NULL)

#define udlist_foreach_rev(list, type, var) \
    for (UDListNode *_ud_n = (list) ? (list)->tail : NULL; _ud_n; _ud_n = _ud_n->prev) \
        for (type *var = (type *)_ud_n->data; var; var = NULL)

/* ============================================================
 * UStack — Pile LIFO (backed by UVec)
 * ============================================================ */

typedef struct {
    UVec *vec;
    int gc_managed;
} UStack;

UStack *ustack_new(size_t elem_size);
UStack *ustack_new_gc(size_t elem_size);
void    ustack_free(UStack *s);
void    ustack_push(UStack *s, const void *elem);
void   *ustack_peek(const UStack *s);
int     ustack_pop(UStack *s);
int     ustack_pop_into(UStack *s, void *out);
int     ustack_size(const UStack *s);
int     ustack_empty(const UStack *s);
void    ustack_clear(UStack *s);

#define ustack_push_val(s, val) \
    do { __typeof__(val) _us_tmp = (val); ustack_push((s), &_us_tmp); } while(0)

#define ustack_peek_as(s, type) (*(type *)ustack_peek((s)))

/* ============================================================
 * UQueue — File FIFO (backed by UDList)
 * ============================================================ */

typedef struct {
    UDList *list;
    int gc_managed;
} UQueue;

UQueue *uqueue_new(size_t elem_size);
UQueue *uqueue_new_gc(size_t elem_size);
void    uqueue_free(UQueue *q);
void    uqueue_enqueue(UQueue *q, const void *elem);
void   *uqueue_peek(const UQueue *q);
int     uqueue_dequeue(UQueue *q);
int     uqueue_dequeue_into(UQueue *q, void *out);
int     uqueue_size(const UQueue *q);
int     uqueue_empty(const UQueue *q);
void    uqueue_clear(UQueue *q);

#define uqueue_enqueue_val(q, val) \
    do { __typeof__(val) _uq_tmp = (val); uqueue_enqueue((q), &_uq_tmp); } while(0)

#define uqueue_peek_as(q, type) (*(type *)uqueue_peek((q)))

/* ============================================================
 * UDeque — File double (backed by UDList)
 * ============================================================ */

typedef struct {
    UDList *list;
    int gc_managed;
} UDeque;

UDeque *udeque_new(size_t elem_size);
UDeque *udeque_new_gc(size_t elem_size);
void    udeque_free(UDeque *d);
void    udeque_push_front(UDeque *d, const void *elem);
void    udeque_push_back(UDeque *d, const void *elem);
void   *udeque_front(const UDeque *d);
void   *udeque_back(const UDeque *d);
int     udeque_pop_front(UDeque *d);
int     udeque_pop_back(UDeque *d);
int     udeque_pop_front_into(UDeque *d, void *out);
int     udeque_pop_back_into(UDeque *d, void *out);
int     udeque_size(const UDeque *d);
int     udeque_empty(const UDeque *d);

#define udeque_push_front_val(d, val) \
    do { __typeof__(val) _ude_tmp = (val); udeque_push_front((d), &_ude_tmp); } while(0)

#define udeque_push_back_val(d, val) \
    do { __typeof__(val) _ude_tmp = (val); udeque_push_back((d), &_ude_tmp); } while(0)

/* ============================================================
 * UStrList — Liste de strings (copies automatiques)
 * ============================================================ */

typedef struct {
    char **data;
    int size;
    int capacity;
    int gc_managed;
} UStrList;

UStrList   *ustrlist_new(void);
UStrList   *ustrlist_new_gc(void);
void        ustrlist_free(UStrList *list);
void        ustrlist_add(UStrList *list, const char *str);
void        ustrlist_insert(UStrList *list, int index, const char *str);
const char *ustrlist_get(const UStrList *list, int index);
int         ustrlist_size(const UStrList *list);
int         ustrlist_empty(const UStrList *list);
int         ustrlist_remove(UStrList *list, int index);
void        ustrlist_clear(UStrList *list);
int         ustrlist_find(const UStrList *list, const char *str);
int         ustrlist_contains(const UStrList *list, const char *str);
void        ustrlist_sort(UStrList *list);
void        ustrlist_sort_fn(UStrList *list, uvec_cmp_fn cmp);
char       *ustrlist_join(const UStrList *list, const char *sep);
UStrList   *ustrlist_from_split(const char *str, const char *delim);

#define ustrlist_foreach(list, var) \
    for (int _usl_i = 0; _usl_i < ustrlist_size(list); _usl_i++) \
        for (const char *var = ustrlist_get((list), _usl_i); var; var = NULL)

/* ============================================================
 * Header-only mode
 * ============================================================ */

#ifdef ULIST_IMPLEMENTATION
/* Future: inline implementations here */
#endif

#endif /* ULIST_H */
