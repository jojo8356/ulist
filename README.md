# UList

Generic data structures for C — no boilerplate, no pain.

UList provides **7 ready-to-use data structures** with a clean, consistent API. Just `#include "ulist.h"`, link with `-lulist`, and you're done.

## Data Structures

| Structure | Type | Description |
|-----------|------|-------------|
| **UVec** | Dynamic array | Like `ArrayList` / `std::vector` — contiguous, fast random access |
| **ULinked** | Singly linked list | Push/pop from head, append to tail |
| **UDList** | Doubly linked list | O(1) push/pop from both ends, bidirectional traversal |
| **UStack** | LIFO stack | Backed by UVec |
| **UQueue** | FIFO queue | Backed by UDList |
| **UDeque** | Double-ended queue | Push/pop from both ends, backed by UDList |
| **UStrList** | String list | Automatic copy semantics, join, split |

All structures are **type-generic** via `void*` + `elem_size`, with **convenience macros** to avoid boilerplate.

## Quick Start

### Install

```bash
git clone https://github.com/jojo8356/ulist.git
cd ulist
make build
```

This produces `build/libulist.a`. Copy it along with `include/ulist.h` into your project.

### Compile your program

```bash
gcc -Ipath/to/ulist/include main.c -Lpath/to/ulist/build -lulist -o main
```

## Usage

### UVec — Dynamic Array

```c
#include "ulist.h"

int main(void)
{
    UVec *v = uvec_new(sizeof(int));

    // Add elements
    uvec_add_val(v, 42);
    uvec_add_val(v, 17);
    uvec_add_val(v, 99);

    // Access
    int first = uvec_get_as(v, 0, int);   // 42
    int last  = *(int *)uvec_last(v);      // 99
    int size  = uvec_size(v);              // 3

    // Insert at position
    uvec_insert_val(v, 1, 50);  // [42, 50, 17, 99]

    // Modify
    uvec_set_val(v, 0, 100);    // [100, 50, 17, 99]

    // Search
    int key = 17;
    int idx = uvec_find(v, &key);          // 2
    int has = uvec_contains(v, &key);      // 1

    // Sort
    uvec_sort(v, uvec_cmp_int);            // [17, 50, 99, 100]
    uvec_sort(v, uvec_cmp_int_desc);       // [100, 99, 50, 17]

    // Iterate
    uvec_foreach(v, int, elem) {
        printf("%d\n", *elem);
    }

    // Remove
    uvec_remove(v, 1);       // remove at index (shifts elements)
    uvec_remove_fast(v, 0);  // remove at index (swaps with last — O(1))
    uvec_pop(v);             // remove last element

    uvec_free(v);
}
```

### UVec — Functional Style

```c
// Filter: keep only even numbers
int is_even(const void *elem, void *ctx) {
    (void)ctx;
    return (*(const int *)elem) % 2 == 0;
}
UVec *evens = uvec_filter(v, is_even, NULL);

// Map: int → double (multiply by 2.0)
void times_two(const void *in, void *out, void *ctx) {
    (void)ctx;
    *(double *)out = *(const int *)in * 2.0;
}
UVec *doubled = uvec_map(v, sizeof(double), times_two, NULL);

// Reduce: sum all elements
void sum_fn(const void *elem, void *acc) {
    *(int *)acc += *(const int *)elem;
}
int total = 0;
uvec_reduce(v, sum_fn, &total);

// Each: iterate with index
void print_elem(const void *elem, int i, void *ctx) {
    (void)ctx;
    printf("[%d] = %d\n", i, *(const int *)elem);
}
uvec_each(v, print_elem, NULL);

// Clone & Reverse
UVec *copy = uvec_clone(v);
uvec_reverse(copy);

// Don't forget to free allocated results
uvec_free(evens);
uvec_free(doubled);
uvec_free(copy);
```

### ULinked — Singly Linked List

```c
ULinked *list = ulinked_new(sizeof(int));

ulinked_push_val(list, 3);    // push to head: [3]
ulinked_push_val(list, 2);    // [2, 3]
ulinked_push_val(list, 1);    // [1, 2, 3]
ulinked_append_val(list, 4);  // append to tail: [1, 2, 3, 4]

int head = ulinked_head_as(list, int);  // 1

ulinked_sort(list, uvec_cmp_int);
ulinked_reverse(list);

ulinked_foreach(list, int, elem) {
    printf("%d ", *elem);
}

ulinked_pop(list);       // remove head
ulinked_remove(list, 1); // remove at index

ulinked_free(list);
```

### UDList — Doubly Linked List

```c
UDList *dl = udlist_new(sizeof(int));

udlist_push_back_val(dl, 1);
udlist_push_back_val(dl, 2);
udlist_push_front_val(dl, 0);  // [0, 1, 2]

int front = udlist_front_as(dl, int);  // 0
int back  = udlist_back_as(dl, int);   // 2

// Bidirectional access (auto-optimized)
int mid = *(int *)udlist_get(dl, 1);   // 1

// Insert at any position
udlist_insert(dl, 2, &(int){10});      // [0, 1, 10, 2]

udlist_pop_front(dl);
udlist_pop_back(dl);

// Forward and reverse iteration
udlist_foreach(dl, int, elem)     { printf("%d ", *elem); }
udlist_foreach_rev(dl, int, elem) { printf("%d ", *elem); }

udlist_sort(dl, uvec_cmp_int);
udlist_reverse(dl);

udlist_free(dl);
```

### UStack — LIFO Stack

```c
UStack *s = ustack_new(sizeof(int));

ustack_push_val(s, 1);
ustack_push_val(s, 2);
ustack_push_val(s, 3);

int top = ustack_peek_as(s, int);  // 3 (LIFO)

int val;
ustack_pop_into(s, &val);  // val = 3, stack = [1, 2]
ustack_pop(s);              // discard top, stack = [1]

ustack_free(s);
```

### UQueue — FIFO Queue

```c
UQueue *q = uqueue_new(sizeof(int));

uqueue_enqueue_val(q, 1);
uqueue_enqueue_val(q, 2);
uqueue_enqueue_val(q, 3);

int front = uqueue_peek_as(q, int);  // 1 (FIFO)

int val;
uqueue_dequeue_into(q, &val);  // val = 1, queue = [2, 3]
uqueue_dequeue(q);              // discard front, queue = [3]

uqueue_free(q);
```

### UDeque — Double-Ended Queue

```c
UDeque *d = udeque_new(sizeof(int));

udeque_push_back_val(d, 2);
udeque_push_front_val(d, 1);
udeque_push_back_val(d, 3);   // [1, 2, 3]

int f = *(int *)udeque_front(d);  // 1
int b = *(int *)udeque_back(d);   // 3

int val;
udeque_pop_front_into(d, &val);  // val = 1
udeque_pop_back_into(d, &val);   // val = 3

udeque_free(d);
```

### UStrList — String List

```c
UStrList *sl = ustrlist_new();

// Add strings (automatically copied)
ustrlist_add(sl, "hello");
ustrlist_add(sl, "world");
ustrlist_add(sl, "foo");

// Access
const char *s = ustrlist_get(sl, 0);  // "hello"
int idx = ustrlist_find(sl, "world"); // 1

// Sort alphabetically
ustrlist_sort(sl);

// Join
char *joined = ustrlist_join(sl, ", ");  // "foo, hello, world"
free(joined);

// Split a string
UStrList *parts = ustrlist_from_split("a,b,c", ",");
// parts = ["a", "b", "c"]

// Iterate
ustrlist_foreach(sl, s) {
    printf("%s\n", s);
}

ustrlist_free(sl);
ustrlist_free(parts);
```

## Built-in Comparators

Use these with `uvec_sort()`, `ulinked_sort()`, `udlist_sort()`:

| Function | Description |
|----------|-------------|
| `uvec_cmp_int` | `int` ascending |
| `uvec_cmp_int_desc` | `int` descending |
| `uvec_cmp_double` | `double` ascending |
| `uvec_cmp_str` | `char*` alphabetical (strcmp) |

## Macros Reference

### Value macros (pass values directly, no `&` needed)

```c
uvec_add_val(v, 42);
uvec_insert_val(v, 0, 42);
uvec_set_val(v, 0, 42);
ulinked_push_val(list, 42);
ulinked_append_val(list, 42);
udlist_push_front_val(dl, 42);
udlist_push_back_val(dl, 42);
ustack_push_val(s, 42);
uqueue_enqueue_val(q, 42);
udeque_push_front_val(d, 42);
udeque_push_back_val(d, 42);
```

### Access macros (cast to type)

```c
int x = uvec_get_as(v, 0, int);
int *p = uvec_get_typed(v, 0, int);
int h = ulinked_head_as(list, int);
int f = udlist_front_as(dl, int);
int b = udlist_back_as(dl, int);
int t = ustack_peek_as(s, int);
int q = uqueue_peek_as(q, int);
```

### Iteration macros

```c
uvec_foreach(v, int, elem)           { printf("%d\n", *elem); }
ulinked_foreach(list, int, elem)     { printf("%d\n", *elem); }
udlist_foreach(dl, int, elem)        { printf("%d\n", *elem); }
udlist_foreach_rev(dl, int, elem)    { printf("%d\n", *elem); }
ustrlist_foreach(sl, s)              { printf("%s\n", s); }
```

## Build Targets

```bash
make build          # build libulist.a (release, -O2)
make debug          # build libulist.a (debug, -g -O0)
make test           # run all 90 tests
make test-asan      # tests with AddressSanitizer + UBSan
make test-valgrind  # tests with Valgrind (leak check)
make analyze        # Clang Static Analyzer
make check          # all of the above (asan + valgrind + analyze)
make clean          # remove build/
make demo           # compile and run example
```

## Requirements

- **GCC** or **Clang** (C11)
- **Make**
- **Valgrind** (optional, for `make test-valgrind`)
- **scan-build** (optional, for `make analyze`)

## Project Structure

```
ulist/
├── include/
│   ├── ulist.h        # public header (all structures + macros)
│   └── ugc.h          # GC header (stubs for now)
├── src/
│   ├── uvec.c         # UVec implementation
│   ├── ulinked.c      # ULinked implementation
│   ├── udlist.c        # UDList implementation
│   ├── ustack.c        # UStack implementation
│   ├── uqueue.c        # UQueue implementation
│   ├── udeque.c        # UDeque implementation
│   ├── ustrlist.c      # UStrList implementation
│   └── ugc.c           # GC stubs
├── tests/
│   ├── utest.h         # mini test framework
│   ├── test_main.c     # test runner
│   ├── test_uvec.c     # UVec tests (24)
│   ├── test_ulinked.c  # ULinked tests (12)
│   ├── test_udlist.c   # UDList tests (14)
│   ├── test_ustack.c   # UStack tests (8)
│   ├── test_uqueue.c   # UQueue tests (8)
│   ├── test_udeque.c   # UDeque tests (8)
│   └── test_ustrlist.c # UStrList tests (16)
├── examples/
│   └── demo.c
├── Makefile
└── README.md
```

## License

MIT
