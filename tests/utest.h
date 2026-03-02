#ifndef UTEST_H
#define UTEST_H

#include <stdio.h>
#include <string.h>

/* ============================================================
 * utest — Mini test framework for UList
 *
 * Usage: define UTEST_MAIN in exactly one .c file before including,
 *        then #include your test files, then call TEST() + TEST_REPORT().
 * ============================================================ */

static int utest_pass_count = 0;
static int utest_fail_count = 0;
static int utest_current_failed = 0;

#define ASSERT(expr, msg)                                               \
    do {                                                                \
        if (!(expr)) {                                                  \
            printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, (msg));    \
            utest_current_failed = 1;                                   \
        }                                                               \
    } while (0)

#define ASSERT_EQ(a, b, msg)                                            \
    do {                                                                \
        if ((a) != (b)) {                                               \
            printf("  FAIL %s:%d: %s (got %d, expected %d)\n",          \
                   __FILE__, __LINE__, (msg), (int)(a), (int)(b));      \
            utest_current_failed = 1;                                   \
        }                                                               \
    } while (0)

#define ASSERT_NOT_NULL(ptr, msg)                                       \
    do {                                                                \
        if ((ptr) == NULL) {                                            \
            printf("  FAIL %s:%d: %s (got NULL)\n",                     \
                   __FILE__, __LINE__, (msg));                          \
            utest_current_failed = 1;                                   \
        }                                                               \
    } while (0)

#define TEST(name)                                                      \
    do {                                                                \
        utest_current_failed = 0;                                       \
        printf("  RUN  %s\n", #name);                                   \
        name();                                                         \
        if (utest_current_failed) {                                     \
            utest_fail_count++;                                         \
        } else {                                                        \
            utest_pass_count++;                                         \
            printf("  OK   %s\n", #name);                               \
        }                                                               \
    } while (0)

#define TEST_REPORT()                                                   \
    do {                                                                \
        printf("\n========================================\n");          \
        printf("  %d passed, %d failed\n",                              \
               utest_pass_count, utest_fail_count);                     \
        printf("========================================\n");            \
    } while (0)

#define TEST_EXIT_CODE() (utest_fail_count > 0 ? 1 : 0)

#endif /* UTEST_H */
