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

## Garbage Collector (opt-in)

UList ships **UGC**, a **conservative** mark-and-sweep garbage collector,
in the spirit of the big GCs (Java, Go, Boehm-Demers-Weiser): **when a
variable stops being used, its memory is freed automatically** — no `free()`.

### Zero-free structures

Every structure has a `*_new_gc()` variant. The stack is scanned at each
collection: **any local variable pointing to a tracked object keeps it
alive**, along with its whole content graph.

```c
#include "ulist.h"
#include "ugc.h"

int main(void)
{
    ugc_init();                             // early = best stack capture

    {
        UVec *v = uvec_new_gc(sizeof(int)); // tracked, no free needed
        for (int i = 0; i < 1000; i++)
            uvec_add_val(v, i);
        ugc_collect();                      // v lives on the stack: survives
    }                                       // v is no longer referenced

    ugc_collect();                          // struct + buffer reclaimed
    ugc_shutdown();                         // frees whatever remains
}
```

### Tracked raw allocations

```c
char *s = ugc_calloc(64, 1);        // tracked; the local var anchors it
ugc_collect();                       // survives (young + on-stack)
s = NULL;                            // no longer referenced
ugc_collect();                       // reclaimed — no free() needed

/* GLOBAL variables are NOT stack-scanned: anchor them explicitly */
static char *g_cache;
ugc_add_root((void **)&g_cache);
...
ugc_remove_root((void **)&g_cache);
```

### How it works

- **Automatic roots**: the stack is scanned word by word at each collection
  (registers are flushed first, like Boehm GC does). Any local pointing to
  a tracked block — even via an interior pointer — keeps it alive, along
  with everything reachable from it (conservative marking).
- **Explicit roots** for globals/statics: `ugc_add_root(&ptr)` /
  `ugc_remove_root(&ptr)`.
- **Young objects** are protected for one collection, giving you a window
  to anchor them.
- **Collections**: manual via `ugc_collect()`, or automatic when
  allocations exceed `ugc_set_threshold()` (with `ugc_set_auto(1)`).
  The threshold is adaptive to avoid thrashing on live heaps.
- **Two interchangeable engines** behind the same `ugc.h` API —
  query with `ugc_backend_name()`:
  - `"internal"` (default): hand-rolled, zero-dependency, exact accounting
    — this is what CI builds and tests (`make test`, `make test-asan`).
  - `"boehm"`: the battle-tested Boehm-Demers-Weiser GC (libgc, used by
    Mono/Guile/GCJ) drives the collections — see below.
- **Stats & debug**: `ugc_count()`, `ugc_bytes()`, `ugc_root_count()`,
  `ugc_collections()`, `ugc_dump()`, `ugc_set_verbose()`.
- **Iterative marker**: no recursion — deep chains can't overflow the stack.

### Boehm-Demers-Weiser backend (optional)

```bash
# 1) Build libgc locally (gc-8.x sources): statically disable threading is
#    fine; UGC accounting requires the DISCLAIMER API:
#      cc -O2 -DGC_NOT_DLL -DALL_INTERIOR_POINTERS -DNO_EXECUTE_PERMISSION \
#         -DENABLE_DISCLAIM -Iinclude -c *.c && ar rcs libgc.a *.o
# 2) Point the Makefile at your prefix (needs include/gc.h + lib/libgc.a):
make test-boehm BOEHM_DIR=/path/to/prefix
```

The Boehm backend (`src/backends/ugc_boehm.c`) maps the same API onto
libgc: same tracked allocations, same exact `ugc_count()`/`ugc_bytes()`
(via a 16-byte header + a per-object disclaim hook), same roots, same
explicit `ugc_collect()`. Differences, inherited from the engine itself:
no young-object grace window (garbage may be reclaimed at the very first
collection), stack-scan toggles are no-ops, and the suite is run without
ASan/LSan (libgc keeps its heap until process exit).

Rules of thumb: anchor objects before losing the last pointer to them; only
store tracked pointers inside other tracked blocks (or roots); `ugc_free()`
on an untracked pointer behaves like `free()`.

## Build Targets

```bash
make build          # build libulist.a (release, -O2)
make debug          # build libulist.a (debug, -g -O0)
make test           # run all 119 tests
make test-asan      # tests with AddressSanitizer + UBSan
make test-boehm    # same 119 tests on the Boehm-Demers-Weiser backend
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
│   └── ugc.h          # public GC header
├── src/
│   ├── uvec.c         # UVec implementation
│   ├── ulinked.c      # ULinked implementation
│   ├── udlist.c        # UDList implementation
│   ├── ustack.c        # UStack implementation
│   ├── uqueue.c        # UQueue implementation
│   ├── udeque.c        # UDeque implementation
│   ├── ustrlist.c      # UStrList implementation
│   ├── ugc.c           # conservative mark-and-sweep GC (internal backend)
│   ├── ugc_alloc.h     # internal alloc router (GC vs classic)
│   └── backends/
│       └── ugc_boehm.c # optional Boehm-Demers-Weiser backend (libgc)
├── tests/
│   ├── utest.h         # mini test framework
│   ├── test_main.c     # test runner
│   ├── test_uvec.c     # UVec tests (24)
│   ├── test_ulinked.c  # ULinked tests (12)
│   ├── test_udlist.c   # UDList tests (13)
│   ├── test_ustack.c   # UStack tests (8)
│   ├── test_uqueue.c   # UQueue tests (8)
│   ├── test_udeque.c   # UDeque tests (8)
│   ├── test_ustrlist.c # UStrList tests (17)
│   └── test_ugc.c      # UGC tests (29)
├── examples/
│   └── demo.c
├── Makefile
└── README.md
```

## License

MIT
