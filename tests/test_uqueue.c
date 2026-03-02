/* test_uqueue.c — included from test_main.c */

static void test_uqueue_new_free(void)
{
    UQueue *q = uqueue_new(sizeof(int));
    ASSERT_NOT_NULL(q, "uqueue_new non-NULL");
    ASSERT_EQ(uqueue_size(q), 0, "new queue size 0");
    ASSERT(uqueue_empty(q), "new queue empty");
    uqueue_free(q);
}

static void test_uqueue_enqueue_dequeue(void)
{
    UQueue *q = uqueue_new(sizeof(int));
    uqueue_enqueue_val(q, 1);
    uqueue_enqueue_val(q, 2);
    uqueue_enqueue_val(q, 3);
    ASSERT_EQ(uqueue_size(q), 3, "size 3");
    ASSERT_EQ(uqueue_peek_as(q, int), 1, "peek should be 1 (FIFO)");
    ASSERT(uqueue_dequeue(q), "dequeue should return 1");
    ASSERT_EQ(uqueue_peek_as(q, int), 2, "peek after dequeue should be 2");
    uqueue_free(q);
}

static void test_uqueue_dequeue_into(void)
{
    UQueue *q = uqueue_new(sizeof(int));
    uqueue_enqueue_val(q, 42);
    int val = 0;
    ASSERT(uqueue_dequeue_into(q, &val), "dequeue_into should return 1");
    ASSERT_EQ(val, 42, "dequeued value should be 42");
    ASSERT(uqueue_empty(q), "queue should be empty");
    ASSERT(!uqueue_dequeue_into(q, &val), "dequeue_into on empty should return 0");
    uqueue_free(q);
}

static void test_uqueue_clear(void)
{
    UQueue *q = uqueue_new(sizeof(int));
    for (int i = 0; i < 10; i++)
        uqueue_enqueue_val(q, i);
    uqueue_clear(q);
    ASSERT(uqueue_empty(q), "empty after clear");
    uqueue_free(q);
}

static void test_uqueue_dequeue_empty(void)
{
    UQueue *q = uqueue_new(sizeof(int));
    ASSERT(!uqueue_dequeue(q), "dequeue on empty should return 0");
    ASSERT(uqueue_peek(q) == NULL, "peek on empty should be NULL");
    uqueue_free(q);
}

static void test_uqueue_fifo(void)
{
    UQueue *q = uqueue_new(sizeof(int));
    for (int i = 1; i <= 5; i++)
        uqueue_enqueue_val(q, i);
    for (int i = 1; i <= 5; i++) {
        int val = 0;
        uqueue_dequeue_into(q, &val);
        ASSERT_EQ(val, i, "FIFO order");
    }
    uqueue_free(q);
}

static void test_uqueue_gc(void)
{
    UQueue *q = uqueue_new_gc(sizeof(int));
    ASSERT_EQ(q->gc_managed, 1, "gc_managed");
    uqueue_enqueue_val(q, 99);
    ASSERT_EQ(uqueue_peek_as(q, int), 99, "gc queue works");
    uqueue_free(q);
}

static void test_uqueue_free_null(void)
{
    uqueue_free(NULL);
    ASSERT(1, "uqueue_free(NULL) ok");
}
