/* test_uvec.c — included from test_main.c */

/* --- Création / Destruction --- */

static void test_uvec_new_free(void)
{
    UVec *v = uvec_new(sizeof(int));
    ASSERT_NOT_NULL(v, "uvec_new should return non-NULL");
    ASSERT_EQ(uvec_size(v), 0, "new UVec should have size 0");
    ASSERT(uvec_empty(v), "new UVec should be empty");
    ASSERT_EQ(v->capacity, UVEC_INITIAL_CAPACITY, "initial capacity should be 8");
    ASSERT_EQ(v->gc_managed, 0, "manual UVec should not be gc_managed");
    uvec_free(v);
}

static void test_uvec_new_gc(void)
{
    UVec *v = uvec_new_gc(sizeof(int));
    ASSERT_NOT_NULL(v, "uvec_new_gc should return non-NULL");
    ASSERT_EQ(v->gc_managed, 1, "GC UVec should be gc_managed");
    uvec_free(v);
}

static void test_uvec_free_null(void)
{
    uvec_free(NULL);
    ASSERT(1, "uvec_free(NULL) should not crash");
}

/* --- Ajout / Accès --- */

static void test_uvec_add_get(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 0; i < 20; i++)
        uvec_add_val(v, i);
    ASSERT_EQ(uvec_size(v), 20, "size should be 20 after adding 20 elements");
    ASSERT(!uvec_empty(v), "should not be empty");
    for (int i = 0; i < 20; i++)
        ASSERT_EQ(uvec_get_as(v, i, int), i, "element should match");
    uvec_free(v);
}

static void test_uvec_first_last(void)
{
    UVec *v = uvec_new(sizeof(int));
    ASSERT(uvec_first(v) == NULL, "first on empty should be NULL");
    ASSERT(uvec_last(v) == NULL, "last on empty should be NULL");
    uvec_add_val(v, 10);
    uvec_add_val(v, 20);
    uvec_add_val(v, 30);
    ASSERT_EQ(*(int *)uvec_first(v), 10, "first should be 10");
    ASSERT_EQ(*(int *)uvec_last(v), 30, "last should be 30");
    uvec_free(v);
}

static void test_uvec_insert(void)
{
    UVec *v = uvec_new(sizeof(int));
    uvec_add_val(v, 1);
    uvec_add_val(v, 3);
    uvec_insert_val(v, 1, 2);
    ASSERT_EQ(uvec_size(v), 3, "size should be 3");
    ASSERT_EQ(uvec_get_as(v, 0, int), 1, "elem 0 should be 1");
    ASSERT_EQ(uvec_get_as(v, 1, int), 2, "elem 1 should be 2");
    ASSERT_EQ(uvec_get_as(v, 2, int), 3, "elem 2 should be 3");
    /* Insert at beginning */
    uvec_insert_val(v, 0, 0);
    ASSERT_EQ(uvec_get_as(v, 0, int), 0, "insert at 0");
    /* Insert at end */
    uvec_insert_val(v, 4, 4);
    ASSERT_EQ(uvec_get_as(v, 4, int), 4, "insert at end");
    uvec_free(v);
}

/* --- Modification --- */

static void test_uvec_set(void)
{
    UVec *v = uvec_new(sizeof(int));
    uvec_add_val(v, 1);
    uvec_add_val(v, 2);
    uvec_set_val(v, 1, 42);
    ASSERT_EQ(uvec_get_as(v, 1, int), 42, "set should update element");
    uvec_free(v);
}

/* --- Suppression --- */

static void test_uvec_remove(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        uvec_add_val(v, i);
    ASSERT(uvec_remove(v, 2), "remove should return 1");
    ASSERT_EQ(uvec_size(v), 4, "size should be 4 after remove");
    ASSERT_EQ(uvec_get_as(v, 0, int), 0, "elem 0");
    ASSERT_EQ(uvec_get_as(v, 1, int), 1, "elem 1");
    ASSERT_EQ(uvec_get_as(v, 2, int), 3, "elem 2 should be 3 (shifted)");
    ASSERT_EQ(uvec_get_as(v, 3, int), 4, "elem 3");
    ASSERT(!uvec_remove(v, -1), "remove invalid index should return 0");
    ASSERT(!uvec_remove(v, 10), "remove out of bounds should return 0");
    uvec_free(v);
}

static void test_uvec_remove_fast(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        uvec_add_val(v, i);
    /* remove_fast(1): swaps elem[1] with elem[4], then shrinks */
    ASSERT(uvec_remove_fast(v, 1), "remove_fast should return 1");
    ASSERT_EQ(uvec_size(v), 4, "size should be 4");
    ASSERT_EQ(uvec_get_as(v, 0, int), 0, "elem 0 unchanged");
    ASSERT_EQ(uvec_get_as(v, 1, int), 4, "elem 1 should be last element (swapped)");
    uvec_free(v);
}

static void test_uvec_pop(void)
{
    UVec *v = uvec_new(sizeof(int));
    ASSERT(!uvec_pop(v), "pop on empty should return 0");
    uvec_add_val(v, 1);
    uvec_add_val(v, 2);
    ASSERT(uvec_pop(v), "pop should return 1");
    ASSERT_EQ(uvec_size(v), 1, "size should be 1 after pop");
    ASSERT_EQ(uvec_get_as(v, 0, int), 1, "remaining element should be 1");
    uvec_free(v);
}

static void test_uvec_clear(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 0; i < 10; i++)
        uvec_add_val(v, i);
    uvec_clear(v);
    ASSERT_EQ(uvec_size(v), 0, "size should be 0 after clear");
    ASSERT(uvec_empty(v), "should be empty after clear");
    uvec_free(v);
}

/* --- Recherche --- */

static void test_uvec_find_contains(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        uvec_add_val(v, i * 10);
    int key;
    key = 20; ASSERT_EQ(uvec_find(v, &key), 2, "find 20 should return 2");
    key = 99; ASSERT_EQ(uvec_find(v, &key), -1, "find 99 should return -1");
    key = 30; ASSERT(uvec_contains(v, &key), "contains 30 should be true");
    key = 99; ASSERT(!uvec_contains(v, &key), "contains 99 should be false");
    uvec_free(v);
}

static int pred_even(const void *elem, void *ctx)
{
    (void)ctx;
    return (*(const int *)elem) % 2 == 0;
}

static void test_uvec_find_fn(void)
{
    UVec *v = uvec_new(sizeof(int));
    uvec_add_val(v, 1);
    uvec_add_val(v, 3);
    uvec_add_val(v, 4);
    uvec_add_val(v, 7);
    ASSERT_EQ(uvec_find_fn(v, pred_even, NULL), 2, "first even is at index 2");
    uvec_free(v);
}

/* --- Tri --- */

static void test_uvec_sort(void)
{
    UVec *v = uvec_new(sizeof(int));
    uvec_add_val(v, 5);
    uvec_add_val(v, 2);
    uvec_add_val(v, 8);
    uvec_add_val(v, 1);
    uvec_add_val(v, 3);
    uvec_sort(v, uvec_cmp_int);
    ASSERT_EQ(uvec_get_as(v, 0, int), 1, "sorted[0]");
    ASSERT_EQ(uvec_get_as(v, 1, int), 2, "sorted[1]");
    ASSERT_EQ(uvec_get_as(v, 2, int), 3, "sorted[2]");
    ASSERT_EQ(uvec_get_as(v, 3, int), 5, "sorted[3]");
    ASSERT_EQ(uvec_get_as(v, 4, int), 8, "sorted[4]");

    uvec_sort(v, uvec_cmp_int_desc);
    ASSERT_EQ(uvec_get_as(v, 0, int), 8, "desc sorted[0]");
    ASSERT_EQ(uvec_get_as(v, 4, int), 1, "desc sorted[4]");
    uvec_free(v);
}

static void test_uvec_sort_double(void)
{
    UVec *v = uvec_new(sizeof(double));
    uvec_add_val(v, 3.14);
    uvec_add_val(v, 1.41);
    uvec_add_val(v, 2.71);
    uvec_sort(v, uvec_cmp_double);
    ASSERT(uvec_get_as(v, 0, double) < 1.42, "sorted double[0]");
    ASSERT(uvec_get_as(v, 2, double) > 3.13, "sorted double[2]");
    uvec_free(v);
}

/* --- Reverse --- */

static void test_uvec_reverse(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        uvec_add_val(v, i);
    uvec_reverse(v);
    ASSERT_EQ(uvec_get_as(v, 0, int), 4, "reversed[0]");
    ASSERT_EQ(uvec_get_as(v, 4, int), 0, "reversed[4]");
    uvec_free(v);
}

/* --- Clone --- */

static void test_uvec_clone(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        uvec_add_val(v, i);
    UVec *c = uvec_clone(v);
    ASSERT_NOT_NULL(c, "clone should not be NULL");
    ASSERT_EQ(uvec_size(c), 5, "clone size");
    for (int i = 0; i < 5; i++)
        ASSERT_EQ(uvec_get_as(c, i, int), i, "clone element");
    /* Modify original, clone should not change */
    uvec_set_val(v, 0, 99);
    ASSERT_EQ(uvec_get_as(c, 0, int), 0, "clone should be independent");
    uvec_free(v);
    uvec_free(c);
}

/* --- Fonctionnel --- */

static void test_each_fn(const void *elem, int index, void *ctx)
{
    int *sum = (int *)ctx;
    (void)index;
    *sum += *(const int *)elem;
}

static void test_uvec_each(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 1; i <= 5; i++)
        uvec_add_val(v, i);
    int sum = 0;
    uvec_each(v, test_each_fn, &sum);
    ASSERT_EQ(sum, 15, "each should sum to 15");
    uvec_free(v);
}

static void test_uvec_filter(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 0; i < 10; i++)
        uvec_add_val(v, i);
    UVec *evens = uvec_filter(v, pred_even, NULL);
    ASSERT_EQ(uvec_size(evens), 5, "filter should have 5 even numbers");
    ASSERT_EQ(uvec_get_as(evens, 0, int), 0, "evens[0]");
    ASSERT_EQ(uvec_get_as(evens, 1, int), 2, "evens[1]");
    ASSERT_EQ(uvec_get_as(evens, 4, int), 8, "evens[4]");
    uvec_free(v);
    uvec_free(evens);
}

static void test_map_fn(const void *in, void *out, void *ctx)
{
    (void)ctx;
    *(double *)out = (double)(*(const int *)in) * 2.0;
}

static void test_uvec_map(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 1; i <= 3; i++)
        uvec_add_val(v, i);
    UVec *mapped = uvec_map(v, sizeof(double), test_map_fn, NULL);
    ASSERT_EQ(uvec_size(mapped), 3, "mapped size");
    ASSERT(uvec_get_as(mapped, 0, double) > 1.9, "mapped[0]");
    ASSERT(uvec_get_as(mapped, 1, double) > 3.9, "mapped[1]");
    ASSERT(uvec_get_as(mapped, 2, double) > 5.9, "mapped[2]");
    uvec_free(v);
    uvec_free(mapped);
}

static void test_reduce_fn(const void *elem, void *accumulator)
{
    *(int *)accumulator += *(const int *)elem;
}

static void test_uvec_reduce(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 1; i <= 10; i++)
        uvec_add_val(v, i);
    int sum = 0;
    uvec_reduce(v, test_reduce_fn, &sum);
    ASSERT_EQ(sum, 55, "reduce sum of 1..10 should be 55");
    uvec_free(v);
}

/* --- Macro foreach --- */

static void test_uvec_foreach(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        uvec_add_val(v, i);
    int sum = 0;
    uvec_foreach(v, int, elem) {
        sum += *elem;
    }
    ASSERT_EQ(sum, 10, "foreach sum should be 10");
    uvec_free(v);
}

/* --- Get out of bounds --- */

static void test_uvec_get_bounds(void)
{
    UVec *v = uvec_new(sizeof(int));
    ASSERT(uvec_get(v, 0) == NULL, "get on empty should be NULL");
    ASSERT(uvec_get(v, -1) == NULL, "get negative should be NULL");
    uvec_add_val(v, 42);
    ASSERT(uvec_get(v, 1) == NULL, "get out of bounds should be NULL");
    ASSERT_EQ(uvec_get_as(v, 0, int), 42, "get valid index");
    uvec_free(v);
}

/* --- Grow beyond initial capacity --- */

static void test_uvec_grow(void)
{
    UVec *v = uvec_new(sizeof(int));
    for (int i = 0; i < 100; i++)
        uvec_add_val(v, i);
    ASSERT_EQ(uvec_size(v), 100, "should hold 100 elements");
    ASSERT(v->capacity >= 100, "capacity should have grown");
    for (int i = 0; i < 100; i++)
        ASSERT_EQ(uvec_get_as(v, i, int), i, "elements should be intact after grow");
    uvec_free(v);
}
