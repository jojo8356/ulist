/* test_udlist.c — included from test_main.c */

static void test_udlist_new_free(void)
{
    UDList *list = udlist_new(sizeof(int));
    ASSERT_NOT_NULL(list, "udlist_new should return non-NULL");
    ASSERT_EQ(udlist_size(list), 0, "new list size 0");
    ASSERT(udlist_empty(list), "new list empty");
    udlist_free(list);
}

static void test_udlist_push_front_back(void)
{
    UDList *list = udlist_new(sizeof(int));
    udlist_push_back_val(list, 2);
    udlist_push_back_val(list, 3);
    udlist_push_front_val(list, 1);
    ASSERT_EQ(udlist_size(list), 3, "size 3");
    ASSERT_EQ(udlist_front_as(list, int), 1, "front should be 1");
    ASSERT_EQ(udlist_back_as(list, int), 3, "back should be 3");
    udlist_free(list);
}

static void test_udlist_insert(void)
{
    UDList *list = udlist_new(sizeof(int));
    udlist_push_back_val(list, 1);
    udlist_push_back_val(list, 3);
    udlist_insert(list, 1, &(int){2});
    ASSERT_EQ(udlist_size(list), 3, "size 3 after insert");
    ASSERT_EQ(*(int *)udlist_get(list, 0), 1, "get 0");
    ASSERT_EQ(*(int *)udlist_get(list, 1), 2, "get 1");
    ASSERT_EQ(*(int *)udlist_get(list, 2), 3, "get 2");
    udlist_free(list);
}

static void test_udlist_get(void)
{
    UDList *list = udlist_new(sizeof(int));
    for (int i = 0; i < 10; i++)
        udlist_push_back_val(list, i);
    /* Test access from both ends (optimized in implementation) */
    ASSERT_EQ(*(int *)udlist_get(list, 0), 0, "get first");
    ASSERT_EQ(*(int *)udlist_get(list, 9), 9, "get last");
    ASSERT_EQ(*(int *)udlist_get(list, 5), 5, "get middle");
    ASSERT(udlist_get(list, -1) == NULL, "get invalid");
    ASSERT(udlist_get(list, 10) == NULL, "get out of bounds");
    udlist_free(list);
}

static void test_udlist_pop(void)
{
    UDList *list = udlist_new(sizeof(int));
    udlist_push_back_val(list, 1);
    udlist_push_back_val(list, 2);
    udlist_push_back_val(list, 3);
    ASSERT(udlist_pop_front(list), "pop_front should return 1");
    ASSERT_EQ(udlist_front_as(list, int), 2, "front after pop_front");
    ASSERT(udlist_pop_back(list), "pop_back should return 1");
    ASSERT_EQ(udlist_back_as(list, int), 2, "back after pop_back");
    ASSERT_EQ(udlist_size(list), 1, "size should be 1");
    /* Pop last element */
    udlist_pop_front(list);
    ASSERT(udlist_empty(list), "should be empty");
    ASSERT(udlist_front(list) == NULL, "front on empty");
    ASSERT(udlist_back(list) == NULL, "back on empty");
    udlist_free(list);
}

static void test_udlist_remove(void)
{
    UDList *list = udlist_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        udlist_push_back_val(list, i);
    ASSERT(udlist_remove(list, 2), "remove middle");
    ASSERT_EQ(udlist_size(list), 4, "size 4");
    ASSERT_EQ(*(int *)udlist_get(list, 2), 3, "shifted element");
    udlist_free(list);
}

static void test_udlist_clear(void)
{
    UDList *list = udlist_new(sizeof(int));
    for (int i = 0; i < 10; i++)
        udlist_push_back_val(list, i);
    udlist_clear(list);
    ASSERT(udlist_empty(list), "empty after clear");
    ASSERT(udlist_front(list) == NULL, "no front");
    ASSERT(udlist_back(list) == NULL, "no back");
    udlist_free(list);
}

static void test_udlist_find(void)
{
    UDList *list = udlist_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        udlist_push_back_val(list, i * 10);
    ASSERT_EQ(udlist_find(list, &(int){20}), 2, "find 20");
    ASSERT_EQ(udlist_find(list, &(int){99}), -1, "find 99");
    udlist_free(list);
}

static void test_udlist_sort(void)
{
    UDList *list = udlist_new(sizeof(int));
    udlist_push_back_val(list, 5);
    udlist_push_back_val(list, 1);
    udlist_push_back_val(list, 3);
    udlist_push_back_val(list, 2);
    udlist_push_back_val(list, 4);
    udlist_sort(list, uvec_cmp_int);
    ASSERT_EQ(udlist_front_as(list, int), 1, "sorted front");
    ASSERT_EQ(udlist_back_as(list, int), 5, "sorted back");
    /* Verify order with foreach */
    int prev = -1;
    udlist_foreach(list, int, elem) {
        ASSERT(*elem >= prev, "sort order");
        prev = *elem;
    }
    udlist_free(list);
}

static void test_udlist_reverse(void)
{
    UDList *list = udlist_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        udlist_push_back_val(list, i);
    udlist_reverse(list);
    ASSERT_EQ(udlist_front_as(list, int), 4, "reverse front");
    ASSERT_EQ(udlist_back_as(list, int), 0, "reverse back");
    /* Verify traversal in both directions */
    int expected = 4;
    udlist_foreach(list, int, elem) {
        ASSERT_EQ(*elem, expected, "reverse forward order");
        expected--;
    }
    expected = 0;
    udlist_foreach_rev(list, int, elem) {
        ASSERT_EQ(*elem, expected, "reverse backward order");
        expected++;
    }
    udlist_free(list);
}

static void test_udlist_foreach(void)
{
    UDList *list = udlist_new(sizeof(int));
    for (int i = 1; i <= 5; i++)
        udlist_push_back_val(list, i);
    int sum = 0;
    udlist_foreach(list, int, elem) {
        sum += *elem;
    }
    ASSERT_EQ(sum, 15, "foreach sum 15");
    sum = 0;
    udlist_foreach_rev(list, int, elem) {
        sum += *elem;
    }
    ASSERT_EQ(sum, 15, "foreach_rev sum 15");
    udlist_free(list);
}

static void test_udlist_gc(void)
{
    UDList *list = udlist_new_gc(sizeof(int));
    ASSERT_EQ(list->gc_managed, 1, "gc_managed");
    udlist_push_back_val(list, 42);
    ASSERT_EQ(udlist_front_as(list, int), 42, "gc list works");
    udlist_free(list);
}

static void test_udlist_free_null(void)
{
    udlist_free(NULL);
    ASSERT(1, "udlist_free(NULL) ok");
}
