# ============================================================
# UList — Makefile
# ============================================================

CC       = gcc
AR       = ar

# Directories
SRCDIR   = src
INCDIR   = include
TESTDIR  = tests
BUILDDIR = build
OBJDIR   = $(BUILDDIR)/obj
EXDIR    = examples

# Sources
SRCS     = $(wildcard $(SRCDIR)/*.c)
OBJS     = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))
LIB      = $(BUILDDIR)/libulist.a

# Test sources (single compilation unit: test_main.c includes the rest)
TEST_SRCS = $(TESTDIR)/test_main.c
TEST_BIN  = $(BUILDDIR)/test_runner

# Flags
CFLAGS_BASE  = -std=c11 -Wall -Wextra -Wpedantic -Werror -I$(INCDIR)
CFLAGS_REL   = $(CFLAGS_BASE) -O2
CFLAGS_DBG   = $(CFLAGS_BASE) -g -O0
CFLAGS_ASAN  = $(CFLAGS_DBG) -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS_ASAN = -fsanitize=address,undefined

# ============================================================
# Targets
# ============================================================

.PHONY: build debug test test-asan test-valgrind analyze clean check

# --- Build (release) ---
build: CFLAGS = $(CFLAGS_REL)
build: $(LIB)

# --- Debug ---
debug: CFLAGS = $(CFLAGS_DBG)
debug: $(LIB)

# --- Library ---
$(LIB): $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(AR) rcs $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Test (debug mode) ---
test: CFLAGS = $(CFLAGS_DBG)
test: $(LIB)
	$(CC) $(CFLAGS) $(TEST_SRCS) -L$(BUILDDIR) -lulist -o $(TEST_BIN)
	@echo ""
	@echo "=== Running tests ==="
	@$(TEST_BIN)

# --- Test with AddressSanitizer + UBSan ---
test-asan:
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS_ASAN) $(SRCS) $(TEST_SRCS) $(LDFLAGS_ASAN) -o $(BUILDDIR)/test_asan
	@echo ""
	@echo "=== Running tests (ASan + UBSan) ==="
	@$(BUILDDIR)/test_asan

# --- Test with Valgrind ---
test-valgrind: CFLAGS = $(CFLAGS_DBG)
test-valgrind: $(LIB)
	$(CC) $(CFLAGS) $(TEST_SRCS) -L$(BUILDDIR) -lulist -o $(TEST_BIN)
	@echo ""
	@echo "=== Running tests (Valgrind) ==="
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 --quiet $(TEST_BIN)

# --- Static analysis (Clang) ---
analyze:
	@echo "=== Clang Static Analyzer ==="
	scan-build --status-bugs $(CC) $(CFLAGS_DBG) $(SRCS) $(TEST_SRCS) -o /dev/null

# --- Clean ---
clean:
	rm -rf $(BUILDDIR)

# --- Full check (combo pro) ---
check: test-asan test-valgrind analyze
	@echo ""
	@echo "========================================"
	@echo "  ALL CHECKS PASSED"
	@echo "========================================"

# --- Demo ---
demo: CFLAGS = $(CFLAGS_DBG)
demo: $(LIB)
	$(CC) $(CFLAGS) $(EXDIR)/demo.c -L$(BUILDDIR) -lulist -o $(BUILDDIR)/demo
	@$(BUILDDIR)/demo
