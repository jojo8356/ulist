/* test_udeque.c — included from test_main.c */

static void test_udeque_new_free(void)
{
    UDeque *d = udeque_new(sizeof(int));
    ASSERT_NOT_NULL(d, "udeque_new non-NULL");
    ASSERT_EQ(udeque_size(d), 0, "new deque size 0");
    ASSERT(udeque_empty(d), "new deque empty");
    udeque_free(d);
}

static void test_udeque_push_front_back(void)
{
    UDeque *d = udeque_new(sizeof(int));
    udeque_push_back_val(d, 2);
    udeque_push_back_val(d, 3);
    udeque_push_front_val(d, 1);
    ASSERT_EQ(udeque_size(d), 3, "size 3");
    ASSERT_EQ(*(int *)udeque_front(d), 1, "front should be 1");
    ASSERT_EQ(*(int *)udeque_back(d), 3, "back should be 3");
    udeque_free(d);
}

static void test_udeque_pop_front_back(void)
{
    UDeque *d = udeque_new(sizeof(int));
    udeque_push_back_val(d, 1);
    udeque_push_back_val(d, 2);
    udeque_push_back_val(d, 3);
    ASSERT(udeque_pop_front(d), "pop_front");
    ASSERT_EQ(*(int *)udeque_front(d), 2, "front after pop_front");
    ASSERT(udeque_pop_back(d), "pop_back");
    ASSERT_EQ(*(int *)udeque_back(d), 2, "back after pop_back");
    ASSERT_EQ(udeque_size(d), 1, "size 1");
    udeque_free(d);
}

static void test_udeque_pop_into(void)
{
    UDeque *d = udeque_new(sizeof(int));
    udeque_push_back_val(d, 10);
    udeque_push_back_val(d, 20);
    udeque_push_back_val(d, 30);
    int val = 0;
    ASSERT(udeque_pop_front_into(d, &val), "pop_front_into");
    ASSERT_EQ(val, 10, "front value");
    ASSERT(udeque_pop_back_into(d, &val), "pop_back_into");
    ASSERT_EQ(val, 30, "back value");
    ASSERT_EQ(udeque_size(d), 1, "size 1");
    udeque_free(d);
}

static void test_udeque_empty_ops(void)
{
    UDeque *d = udeque_new(sizeof(int));
    ASSERT(!udeque_pop_front(d), "pop_front on empty");
    ASSERT(!udeque_pop_back(d), "pop_back on empty");
    ASSERT(udeque_front(d) == NULL, "front on empty");
    ASSERT(udeque_back(d) == NULL, "back on empty");
    int val = 0;
    ASSERT(!udeque_pop_front_into(d, &val), "pop_front_into on empty");
    ASSERT(!udeque_pop_back_into(d, &val), "pop_back_into on empty");
    udeque_free(d);
}

static void test_udeque_gc(void)
{
    UDeque *d = udeque_new_gc(sizeof(int));
    ASSERT_EQ(d->gc_managed, 1, "gc_managed");
    udeque_push_back_val(d, 42);
    ASSERT_EQ(*(int *)udeque_front(d), 42, "gc deque works");
    udeque_free(d);
}

static void test_udeque_free_null(void)
{
    udeque_free(NULL);
    ASSERT(1, "udeque_free(NULL) ok");
}

static void test_udeque_mixed(void)
{
    UDeque *d = udeque_new(sizeof(int));
    /* Simulate using as stack from front */
    udeque_push_front_val(d, 1);
    udeque_push_front_val(d, 2);
    udeque_push_front_val(d, 3);
    /* And as queue from back */
    udeque_push_back_val(d, 10);
    udeque_push_back_val(d, 20);
    ASSERT_EQ(udeque_size(d), 5, "mixed size");
    ASSERT_EQ(*(int *)udeque_front(d), 3, "mixed front");
    ASSERT_EQ(*(int *)udeque_back(d), 20, "mixed back");
    udeque_free(d);
}
