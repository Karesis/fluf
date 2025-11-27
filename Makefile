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


# ... (保留你原有的 Build 配置和规则) ...

# === Installation Configuration ===

# 默认安装到系统级目录 (通常需要 sudo)
PREFIX ?= /usr/local
INSTALL_INC := $(PREFIX)/include
INSTALL_LIB := $(PREFIX)/lib

# 自动获取 include 下的所有顶级目录名 (例如: std core fs env ...)
# 这样不仅通用，还能用于冲突检测
HEADER_DIRS := $(shell ls $(SRC_DIR)/../include)

.PHONY: install uninstall update check-conflicts

# === Safety Checks Helpers ===

# 确保是 Root 用户 (用于 install/uninstall)
define require_root
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo "Error: This command must be run as root (e.g., sudo make ...)."; \
		exit 1; \
	fi
endef

# 确保不是 Root 用户 (用于 update，防止 git 权限乱掉)
define require_non_root
	@if [ "$$(id -u)" -eq 0 ]; then \
		echo "Error: Do not run 'make update' as root."; \
		echo "Run it as a normal user to preserve git file permissions."; \
		exit 1; \
	fi
endef

# === Installation ===

install: all check-conflicts
	@echo "[CHECK]    Checking Root Privileges..."
	$(call require_root)
	
	@echo "[CHECK]    Verifying Library..."
	@if [ ! -f $(LIB_TARGET) ]; then \
		echo "Error: Library '$(LIB_TARGET)' not found. Build failed?"; \
		exit 1; \
	fi

	@echo "[INSTALL]  Installing Library to $(INSTALL_LIB)..."
	@mkdir -p $(INSTALL_LIB)
	@cp $(LIB_TARGET) $(INSTALL_LIB)/libfluf.a
	@chmod 644 $(INSTALL_LIB)/libfluf.a

	@echo "[INSTALL]  Installing Headers to $(INSTALL_INC)..."
	@mkdir -p $(INSTALL_INC)
	@# 循环复制 include 下的每个目录
	@for dir in $(HEADER_DIRS); do \
		echo "   -> Copying include/$$dir"; \
		cp -r include/$$dir $(INSTALL_INC)/; \
	done
	
	@echo
	@echo "=== Installation Complete ==="
	@echo "Link with: -lfluf"
	@echo

# === Conflict Detection ===

# 这是你要求的安全检查：
# 如果 /usr/local/include/std 已经存在，脚本会报错并停止。
check-conflicts:
	@echo "[CHECK]    Scanning for path conflicts in $(INSTALL_INC)..."
	@conflict_found=0; \
	for dir in $(HEADER_DIRS); do \
		if [ -d "$(INSTALL_INC)/$$dir" ]; then \
			echo "   [!] WARNING: Target directory '$(INSTALL_INC)/$$dir' already exists!"; \
			conflict_found=1; \
		fi \
	done; \
	if [ $$conflict_found -eq 1 ]; then \
		echo; \
		echo "Error: Path conflicts detected."; \
		echo "This means a folder named 'std', 'core', or 'fs' etc. already exists in your system include path."; \
		echo "If this is a previous installation of fluf, run 'sudo make uninstall' first."; \
		echo "If this belongs to another library (e.g. C++ STL), proceed with caution or uninstall manually."; \
		echo "Aborting install to prevent overwriting."; \
		exit 1; \
	else \
		echo "   [OK] No conflicts found."; \
	fi

# === Uninstallation ===

uninstall:
	@echo "[CHECK]    Checking Root Privileges..."
	$(call require_root)

	@echo "[UNINSTALL] Removing Library..."
	@rm -f $(INSTALL_LIB)/libfluf.a

	@echo "[UNINSTALL] Removing Headers..."
	@# 只删除我们在 HEADER_DIRS 中列出的目录，防止误删整个 include
	@for dir in $(HEADER_DIRS); do \
		if [ -d "$(INSTALL_INC)/$$dir" ]; then \
			echo "   -> Removing $(INSTALL_INC)/$$dir"; \
			rm -rf "$(INSTALL_INC)/$$dir"; \
		fi \
	done

	@echo "=== Uninstallation Complete ==="

# === Update ===

update:
	@echo "[UPDATE]   Starting Project Update..."
	$(call require_non_root)
	
	@echo "[GIT]      Pulling latest changes..."
	@git pull
	
	@echo "[GIT]      Updating submodules (if any)..."
	@git submodule update --init --recursive
	
	@echo "[BUILD]    Rebuilding project..."
	@$(MAKE) all
	
	@echo
	@echo "=== Update Complete ==="
	@echo "Note: The new version is built but NOT installed."
	@echo "Run 'sudo make uninstall' then 'sudo make install' to apply changes system-wide."
