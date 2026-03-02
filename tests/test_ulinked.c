/* test_ulinked.c — included from test_main.c */

static void test_ulinked_new_free(void)
{
    ULinked *list = ulinked_new(sizeof(int));
    ASSERT_NOT_NULL(list, "ulinked_new should return non-NULL");
    ASSERT_EQ(ulinked_size(list), 0, "new list should have size 0");
    ASSERT(ulinked_empty(list), "new list should be empty");
    ulinked_free(list);
}

static void test_ulinked_push_head(void)
{
    ULinked *list = ulinked_new(sizeof(int));
    ulinked_push_val(list, 3);
    ulinked_push_val(list, 2);
    ulinked_push_val(list, 1);
    ASSERT_EQ(ulinked_size(list), 3, "size should be 3");
    ASSERT_EQ(ulinked_head_as(list, int), 1, "head should be 1");
    ulinked_free(list);
}

static void test_ulinked_append(void)
{
    ULinked *list = ulinked_new(sizeof(int));
    ulinked_append_val(list, 1);
    ulinked_append_val(list, 2);
    ulinked_append_val(list, 3);
    ASSERT_EQ(ulinked_size(list), 3, "size should be 3");
    ASSERT_EQ(ulinked_head_as(list, int), 1, "head should be 1 (append preserves order)");
    ulinked_free(list);
}

static void test_ulinked_pop(void)
{
    ULinked *list = ulinked_new(sizeof(int));
    ASSERT(!ulinked_pop(list), "pop on empty should return 0");
    ulinked_push_val(list, 2);
    ulinked_push_val(list, 1);
    ASSERT(ulinked_pop(list), "pop should return 1");
    ASSERT_EQ(ulinked_size(list), 1, "size should be 1");
    ASSERT_EQ(ulinked_head_as(list, int), 2, "head should be 2 after pop");
    ulinked_free(list);
}

static void test_ulinked_remove(void)
{
    ULinked *list = ulinked_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        ulinked_append_val(list, i);
    ASSERT(ulinked_remove(list, 2), "remove index 2");
    ASSERT_EQ(ulinked_size(list), 4, "size should be 4");
    ASSERT(!ulinked_remove(list, -1), "remove invalid");
    ASSERT(!ulinked_remove(list, 10), "remove out of bounds");
    ulinked_free(list);
}

static void test_ulinked_clear(void)
{
    ULinked *list = ulinked_new(sizeof(int));
    for (int i = 0; i < 10; i++)
        ulinked_push_val(list, i);
    ulinked_clear(list);
    ASSERT_EQ(ulinked_size(list), 0, "size should be 0 after clear");
    ASSERT(ulinked_empty(list), "should be empty");
    ulinked_free(list);
}

static void test_ulinked_find(void)
{
    ULinked *list = ulinked_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        ulinked_append_val(list, i * 10);
    int key;
    key = 20; ASSERT_EQ(ulinked_find(list, &key), 2, "find 20");
    key = 99; ASSERT_EQ(ulinked_find(list, &key), -1, "find 99 not found");
    ulinked_free(list);
}

static void test_ulinked_sort(void)
{
    ULinked *list = ulinked_new(sizeof(int));
    ulinked_push_val(list, 5);
    ulinked_push_val(list, 1);
    ulinked_push_val(list, 3);
    ulinked_push_val(list, 2);
    ulinked_push_val(list, 4);
    ulinked_sort(list, uvec_cmp_int);
    int prev = -1;
    ulinked_foreach(list, int, elem) {
        ASSERT(*elem >= prev, "sort: elements should be in order");
        prev = *elem;
    }
    ulinked_free(list);
}

static void test_ulinked_reverse(void)
{
    ULinked *list = ulinked_new(sizeof(int));
    for (int i = 0; i < 5; i++)
        ulinked_append_val(list, i);
    ulinked_reverse(list);
    ASSERT_EQ(ulinked_head_as(list, int), 4, "reverse: head should be 4");
    ulinked_free(list);
}

static void test_ulinked_foreach(void)
{
    ULinked *list = ulinked_new(sizeof(int));
    for (int i = 1; i <= 5; i++)
        ulinked_append_val(list, i);
    int sum = 0;
    ulinked_foreach(list, int, elem) {
        sum += *elem;
    }
    ASSERT_EQ(sum, 15, "foreach sum should be 15");
    ulinked_free(list);
}

static void test_ulinked_gc(void)
{
    ULinked *list = ulinked_new_gc(sizeof(int));
    ASSERT_EQ(list->gc_managed, 1, "should be gc_managed");
    ulinked_push_val(list, 42);
    ASSERT_EQ(ulinked_head_as(list, int), 42, "gc list should work");
    ulinked_free(list);
}

static void test_ulinked_free_null(void)
{
    ulinked_free(NULL);
    ASSERT(1, "ulinked_free(NULL) should not crash");
}
