# === Configuration ===

CC := clang

# -- Flags --

# compiler flags
CFLAGS := -g -Wall -Wextra -std=c23 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
# c preprosser flags
CPPFLAGS := -Iinclude -MMD -MP
# linker libraries
LDLIBS :=
# linker flags
LDFLAGS :=

# === Directories ===

# . (root)
# ├── include
# ├── src
# ├── tests
# └── build
#     ├── bin
#     ├── lib
#     └── obj

SRC_DIR := src
TEST_DIR := tests
BUILD_DIR := build

BIN_DIR := $(BUILD_DIR)/bin
LIB_DIR := $(BUILD_DIR)/lib
OBJ_DIR := $(BUILD_DIR)/obj

# === Source Discovery ===

LIB_SRCS := $(shell find $(SRC_DIR) -name '*.c')
TEST_SRCS := $(wildcard $(TEST_DIR)/test_*.c)

# === Object Derivation ===

# src/std/bump.c -> build/obj/src/std/bump.c
LIB_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/src/%.o,$(LIB_SRCS))
# tests/test_bump.c -> build/obj/tests/test_bump.o
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/tests/%.o,$(TEST_SRCS))
# libfluf.a
LIB_TARGET := $(LIB_DIR)/libfluf.a
# tests/test_bump.c -> build/bin/test_bump
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))
DEPS := $(LIB_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

# === Recipes ===

.PHONY: all
all: $(LIB_TARGET)

# build the lib
# $@ -> target (lib/libfluf.a)
# $^ -> all of the deps ($(LIB_OBJS))
$(LIB_TARGET): $(LIB_OBJS)
	@echo "[MAKE]	AR $@"
	@mkdir -p $(dir $@)
	@ar rcs $@ $^

# rule: how to build .o from .c (used by `build the lib`)
# $< -> the first dep (src/std/bump.c)
$(OBJ_DIR)/src/%.o: $(SRC_DIR)/%.c 
	@echo "[MAKE]	CC $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# rule: how to build .o from .c (used by `link and build a test`)
$(OBJ_DIR)/tests/%.o: $(TEST_DIR)/%.c 
	@echo "[MAKE]	CC $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# === Testing ===

.PHONY: test
test: $(TEST_BINS)
	@echo
	@echo "=== Running All Tests ==="
	@echo
	@$(foreach test,$(TEST_BINS), echo "[TEST]	RUN $(test)"; ./$(test);)
	@echo
	@echo "=== All Tests Passed ==="
	@echo

# rule: how to link and build a test
# bin/test_bump depends on both obj/tests/test_bump.o and lib/libfluf.a
# $< -> the first dep (build/obj/tests/test_bump.o)
$(BIN_DIR)/%: $(OBJ_DIR)/tests/%.o $(LIB_TARGET)
	@echo "[MAKE]	LINK $@"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIB_TARGET) $(LDLIBS)


# === Cleaning ===

.PHONY: clean
clean:
	@echo "[MAKE]	CLEAN"
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)

# === Read DepGraph ===

# `clean` didn't need .d graph
# exclude it to save time
ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

# === Unicode ===

# gen tables.gen for std/unicode
.PHONY: gen-unicode
gen-unicode:
	@echo "[GEN] Updating Unicode Tables..."
	@python3 scripts/gen_unicode.py

# === Versioning Helpers ===

# 1. get the newest tag 
CURRENT_TAG := $(shell git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")

# 2. parse tag version (`v0.3.0` -> MAJOR=0, MINOR=3, PATCH=0) 
VERSION_BITS := $(subst v,,$(CURRENT_TAG))
MAJOR := $(word 1,$(subst ., ,$(VERSION_BITS)))
MINOR := $(word 2,$(subst ., ,$(VERSION_BITS)))
PATCH := $(word 3,$(subst ., ,$(VERSION_BITS)))

# 3. calculate nex version 
NEXT_PATCH := $(shell echo $$(($(PATCH)+1)))
NEW_TAG := v$(MAJOR).$(MINOR).$(NEXT_PATCH)

# 4. default msg value
NOTE ?= Maintenance update

.PHONY: btag

btag:
	@if [ -n "$$(git status --porcelain)" ]; then \
        echo "Error: Working directory is not clean. Commit changes first."; \
        exit 1; \
    fi
	@echo "[RELEASE] Bumping Patch: $(CURRENT_TAG) -> $(NEW_TAG)"
	@echo "[MESSAGE] Release $(NEW_TAG): $(NOTE)"
	git tag -a $(NEW_TAG) -m "Release $(NEW_TAG): $(NOTE)"
	git push origin $(NEW_TAG)
