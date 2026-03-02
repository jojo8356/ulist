/* test_ustack.c — included from test_main.c */

static void test_ustack_new_free(void)
{
    UStack *s = ustack_new(sizeof(int));
    ASSERT_NOT_NULL(s, "ustack_new non-NULL");
    ASSERT_EQ(ustack_size(s), 0, "new stack size 0");
    ASSERT(ustack_empty(s), "new stack empty");
    ustack_free(s);
}

static void test_ustack_push_peek_pop(void)
{
    UStack *s = ustack_new(sizeof(int));
    ustack_push_val(s, 1);
    ustack_push_val(s, 2);
    ustack_push_val(s, 3);
    ASSERT_EQ(ustack_size(s), 3, "size 3");
    ASSERT_EQ(ustack_peek_as(s, int), 3, "peek should be 3 (LIFO)");
    ASSERT(ustack_pop(s), "pop should return 1");
    ASSERT_EQ(ustack_peek_as(s, int), 2, "peek after pop should be 2");
    ASSERT_EQ(ustack_size(s), 2, "size 2");
    ustack_free(s);
}

static void test_ustack_pop_into(void)
{
    UStack *s = ustack_new(sizeof(int));
    ustack_push_val(s, 42);
    int val = 0;
    ASSERT(ustack_pop_into(s, &val), "pop_into should return 1");
    ASSERT_EQ(val, 42, "popped value should be 42");
    ASSERT(ustack_empty(s), "stack should be empty");
    ASSERT(!ustack_pop_into(s, &val), "pop_into on empty should return 0");
    ustack_free(s);
}

static void test_ustack_clear(void)
{
    UStack *s = ustack_new(sizeof(int));
    for (int i = 0; i < 10; i++)
        ustack_push_val(s, i);
    ustack_clear(s);
    ASSERT(ustack_empty(s), "empty after clear");
    ustack_free(s);
}

static void test_ustack_pop_empty(void)
{
    UStack *s = ustack_new(sizeof(int));
    ASSERT(!ustack_pop(s), "pop on empty should return 0");
    ASSERT(ustack_peek(s) == NULL, "peek on empty should be NULL");
    ustack_free(s);
}

static void test_ustack_gc(void)
{
    UStack *s = ustack_new_gc(sizeof(int));
    ASSERT_EQ(s->gc_managed, 1, "gc_managed");
    ustack_push_val(s, 99);
    ASSERT_EQ(ustack_peek_as(s, int), 99, "gc stack works");
    ustack_free(s);
}

static void test_ustack_free_null(void)
{
    ustack_free(NULL);
    ASSERT(1, "ustack_free(NULL) ok");
}

static void test_ustack_lifo(void)
{
    UStack *s = ustack_new(sizeof(int));
    for (int i = 1; i <= 5; i++)
        ustack_push_val(s, i);
    for (int i = 5; i >= 1; i--) {
        int val = 0;
        ustack_pop_into(s, &val);
        ASSERT_EQ(val, i, "LIFO order");
    }
    ustack_free(s);
}
