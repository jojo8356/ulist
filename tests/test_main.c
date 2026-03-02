#include "utest.h"
#include "ulist.h"

/* Include all test files (single compilation unit) */
#include "test_uvec.c"
#include "test_ulinked.c"
#include "test_udlist.c"
#include "test_ustack.c"
#include "test_uqueue.c"
#include "test_udeque.c"
#include "test_ustrlist.c"

int main(void)
{
    printf("UList — Full Test Suite\n");
    printf("========================================\n");

    /* UVec */
    printf("\n--- UVec ---\n");
    TEST(test_uvec_new_free);
    TEST(test_uvec_new_gc);
    TEST(test_uvec_free_null);
    TEST(test_uvec_add_get);
    TEST(test_uvec_first_last);
    TEST(test_uvec_insert);
    TEST(test_uvec_set);
    TEST(test_uvec_remove);
    TEST(test_uvec_remove_fast);
    TEST(test_uvec_pop);
    TEST(test_uvec_clear);
    TEST(test_uvec_find_contains);
    TEST(test_uvec_find_fn);
    TEST(test_uvec_sort);
    TEST(test_uvec_sort_double);
    TEST(test_uvec_reverse);
    TEST(test_uvec_clone);
    TEST(test_uvec_each);
    TEST(test_uvec_filter);
    TEST(test_uvec_map);
    TEST(test_uvec_reduce);
    TEST(test_uvec_foreach);
    TEST(test_uvec_get_bounds);
    TEST(test_uvec_grow);

    /* ULinked */
    printf("\n--- ULinked ---\n");
    TEST(test_ulinked_new_free);
    TEST(test_ulinked_push_head);
    TEST(test_ulinked_append);
    TEST(test_ulinked_pop);
    TEST(test_ulinked_remove);
    TEST(test_ulinked_clear);
    TEST(test_ulinked_find);
    TEST(test_ulinked_sort);
    TEST(test_ulinked_reverse);
    TEST(test_ulinked_foreach);
    TEST(test_ulinked_gc);
    TEST(test_ulinked_free_null);

    /* UDList */
    printf("\n--- UDList ---\n");
    TEST(test_udlist_new_free);
    TEST(test_udlist_push_front_back);
    TEST(test_udlist_insert);
    TEST(test_udlist_get);
    TEST(test_udlist_pop);
    TEST(test_udlist_remove);
    TEST(test_udlist_clear);
    TEST(test_udlist_find);
    TEST(test_udlist_sort);
    TEST(test_udlist_reverse);
    TEST(test_udlist_foreach);
    TEST(test_udlist_gc);
    TEST(test_udlist_free_null);

    /* UStack */
    printf("\n--- UStack ---\n");
    TEST(test_ustack_new_free);
    TEST(test_ustack_push_peek_pop);
    TEST(test_ustack_pop_into);
    TEST(test_ustack_clear);
    TEST(test_ustack_pop_empty);
    TEST(test_ustack_gc);
    TEST(test_ustack_free_null);
    TEST(test_ustack_lifo);

    /* UQueue */
    printf("\n--- UQueue ---\n");
    TEST(test_uqueue_new_free);
    TEST(test_uqueue_enqueue_dequeue);
    TEST(test_uqueue_dequeue_into);
    TEST(test_uqueue_clear);
    TEST(test_uqueue_dequeue_empty);
    TEST(test_uqueue_fifo);
    TEST(test_uqueue_gc);
    TEST(test_uqueue_free_null);

    /* UDeque */
    printf("\n--- UDeque ---\n");
    TEST(test_udeque_new_free);
    TEST(test_udeque_push_front_back);
    TEST(test_udeque_pop_front_back);
    TEST(test_udeque_pop_into);
    TEST(test_udeque_empty_ops);
    TEST(test_udeque_gc);
    TEST(test_udeque_free_null);
    TEST(test_udeque_mixed);

    /* UStrList */
    printf("\n--- UStrList ---\n");
    TEST(test_ustrlist_new_free);
    TEST(test_ustrlist_add_get);
    TEST(test_ustrlist_insert);
    TEST(test_ustrlist_remove);
    TEST(test_ustrlist_clear);
    TEST(test_ustrlist_find_contains);
    TEST(test_ustrlist_sort);
    TEST(test_ustrlist_join);
    TEST(test_ustrlist_join_empty);
    TEST(test_ustrlist_from_split);
    TEST(test_ustrlist_split_multi_delim);
    TEST(test_ustrlist_split_empty_fields);
    TEST(test_ustrlist_foreach);
    TEST(test_ustrlist_gc);
    TEST(test_ustrlist_free_null);
    TEST(test_ustrlist_copy_semantics);
    TEST(test_ustrlist_grow);

    TEST_REPORT();
    return TEST_EXIT_CODE();
}
