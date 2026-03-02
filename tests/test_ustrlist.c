/* test_ustrlist.c — included from test_main.c */

static void test_ustrlist_new_free(void)
{
    UStrList *list = ustrlist_new();
    ASSERT_NOT_NULL(list, "ustrlist_new non-NULL");
    ASSERT_EQ(ustrlist_size(list), 0, "new list size 0");
    ASSERT(ustrlist_empty(list), "new list empty");
    ustrlist_free(list);
}

static void test_ustrlist_add_get(void)
{
    UStrList *list = ustrlist_new();
    ustrlist_add(list, "hello");
    ustrlist_add(list, "world");
    ASSERT_EQ(ustrlist_size(list), 2, "size 2");
    ASSERT(strcmp(ustrlist_get(list, 0), "hello") == 0, "get 0");
    ASSERT(strcmp(ustrlist_get(list, 1), "world") == 0, "get 1");
    ASSERT(ustrlist_get(list, 2) == NULL, "get out of bounds");
    ustrlist_free(list);
}

static void test_ustrlist_insert(void)
{
    UStrList *list = ustrlist_new();
    ustrlist_add(list, "a");
    ustrlist_add(list, "c");
    ustrlist_insert(list, 1, "b");
    ASSERT_EQ(ustrlist_size(list), 3, "size 3");
    ASSERT(strcmp(ustrlist_get(list, 0), "a") == 0, "insert get 0");
    ASSERT(strcmp(ustrlist_get(list, 1), "b") == 0, "insert get 1");
    ASSERT(strcmp(ustrlist_get(list, 2), "c") == 0, "insert get 2");
    ustrlist_free(list);
}

static void test_ustrlist_remove(void)
{
    UStrList *list = ustrlist_new();
    ustrlist_add(list, "a");
    ustrlist_add(list, "b");
    ustrlist_add(list, "c");
    ASSERT(ustrlist_remove(list, 1), "remove");
    ASSERT_EQ(ustrlist_size(list), 2, "size 2");
    ASSERT(strcmp(ustrlist_get(list, 1), "c") == 0, "shifted");
    ASSERT(!ustrlist_remove(list, -1), "remove invalid");
    ustrlist_free(list);
}

static void test_ustrlist_clear(void)
{
    UStrList *list = ustrlist_new();
    ustrlist_add(list, "a");
    ustrlist_add(list, "b");
    ustrlist_clear(list);
    ASSERT(ustrlist_empty(list), "empty after clear");
    ustrlist_free(list);
}

static void test_ustrlist_find_contains(void)
{
    UStrList *list = ustrlist_new();
    ustrlist_add(list, "apple");
    ustrlist_add(list, "banana");
    ustrlist_add(list, "cherry");
    ASSERT_EQ(ustrlist_find(list, "banana"), 1, "find banana");
    ASSERT_EQ(ustrlist_find(list, "grape"), -1, "find grape");
    ASSERT(ustrlist_contains(list, "cherry"), "contains cherry");
    ASSERT(!ustrlist_contains(list, "grape"), "not contains grape");
    ustrlist_free(list);
}

static void test_ustrlist_sort(void)
{
    UStrList *list = ustrlist_new();
    ustrlist_add(list, "cherry");
    ustrlist_add(list, "apple");
    ustrlist_add(list, "banana");
    ustrlist_sort(list);
    ASSERT(strcmp(ustrlist_get(list, 0), "apple") == 0, "sorted 0");
    ASSERT(strcmp(ustrlist_get(list, 1), "banana") == 0, "sorted 1");
    ASSERT(strcmp(ustrlist_get(list, 2), "cherry") == 0, "sorted 2");
    ustrlist_free(list);
}

static void test_ustrlist_join(void)
{
    UStrList *list = ustrlist_new();
    ustrlist_add(list, "one");
    ustrlist_add(list, "two");
    ustrlist_add(list, "three");
    char *joined = ustrlist_join(list, ", ");
    ASSERT(strcmp(joined, "one, two, three") == 0, "join with comma");
    free(joined);

    char *joined2 = ustrlist_join(list, "");
    ASSERT(strcmp(joined2, "onetwothree") == 0, "join with empty sep");
    free(joined2);

    ustrlist_free(list);
}

static void test_ustrlist_join_empty(void)
{
    UStrList *list = ustrlist_new();
    char *joined = ustrlist_join(list, ", ");
    ASSERT(strcmp(joined, "") == 0, "join empty list");
    free(joined);
    ustrlist_free(list);
}

static void test_ustrlist_from_split(void)
{
    UStrList *list = ustrlist_from_split("a,b,c", ",");
    ASSERT_EQ(ustrlist_size(list), 3, "split size");
    ASSERT(strcmp(ustrlist_get(list, 0), "a") == 0, "split 0");
    ASSERT(strcmp(ustrlist_get(list, 1), "b") == 0, "split 1");
    ASSERT(strcmp(ustrlist_get(list, 2), "c") == 0, "split 2");
    ustrlist_free(list);
}

static void test_ustrlist_split_multi_delim(void)
{
    UStrList *list = ustrlist_from_split("hello::world::test", "::");
    ASSERT_EQ(ustrlist_size(list), 3, "split :: size");
    ASSERT(strcmp(ustrlist_get(list, 0), "hello") == 0, "split :: 0");
    ASSERT(strcmp(ustrlist_get(list, 1), "world") == 0, "split :: 1");
    ASSERT(strcmp(ustrlist_get(list, 2), "test") == 0, "split :: 2");
    ustrlist_free(list);
}

static void test_ustrlist_split_empty_fields(void)
{
    UStrList *list = ustrlist_from_split("a,,c", ",");
    ASSERT_EQ(ustrlist_size(list), 3, "split with empty field");
    ASSERT(strcmp(ustrlist_get(list, 0), "a") == 0, "field 0");
    ASSERT(strcmp(ustrlist_get(list, 1), "") == 0, "field 1 empty");
    ASSERT(strcmp(ustrlist_get(list, 2), "c") == 0, "field 2");
    ustrlist_free(list);
}

static void test_ustrlist_foreach(void)
{
    UStrList *list = ustrlist_new();
    ustrlist_add(list, "a");
    ustrlist_add(list, "bb");
    ustrlist_add(list, "ccc");
    int total_len = 0;
    ustrlist_foreach(list, s) {
        total_len += (int)strlen(s);
    }
    ASSERT_EQ(total_len, 6, "foreach total length");
    ustrlist_free(list);
}

static void test_ustrlist_gc(void)
{
    UStrList *list = ustrlist_new_gc();
    ASSERT_EQ(list->gc_managed, 1, "gc_managed");
    ustrlist_add(list, "test");
    ASSERT(strcmp(ustrlist_get(list, 0), "test") == 0, "gc strlist works");
    ustrlist_free(list);
}

static void test_ustrlist_free_null(void)
{
    ustrlist_free(NULL);
    ASSERT(1, "ustrlist_free(NULL) ok");
}

static void test_ustrlist_copy_semantics(void)
{
    UStrList *list = ustrlist_new();
    char buf[32];
    strcpy(buf, "original");
    ustrlist_add(list, buf);
    /* Modify buffer after adding — list should have its own copy */
    strcpy(buf, "modified");
    ASSERT(strcmp(ustrlist_get(list, 0), "original") == 0, "copy semantics");
    ustrlist_free(list);
}

static void test_ustrlist_grow(void)
{
    UStrList *list = ustrlist_new();
    for (int i = 0; i < 50; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "str_%d", i);
        ustrlist_add(list, buf);
    }
    ASSERT_EQ(ustrlist_size(list), 50, "50 strings");
    ASSERT(strcmp(ustrlist_get(list, 49), "str_49") == 0, "last string");
    ustrlist_free(list);
}
