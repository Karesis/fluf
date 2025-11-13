# ===== [fluf] Sane Makefile =====

# --- 1. 配置 (Configuration) ---

# 编译器
CC := clang
# 编译标志
# -g: 调试信息
# -Iinclude: 允许 #include <core/mem/layout.h>
# -Wall -Wextra: 开启所有有用的警告
# -std=c23: C23 标准
CFLAGS := -g -Wall -Wextra -std=c23 -Iinclude -D_POSIX_C_SOURCE=200809L
# 链接标志 (测试时可能需要)
LDLIBS := 
# 链接器标志 (例如 -L/usr/local/lib)
LDFLAGS := 

# --- 2. 目录 (Directories) ---
# . (根目录)
# ├── bin/    (存放可执行的测试)
# ├── lib/    (存放 libfluf.a)
# ├── obj/    (存放 .o 文件)
# ├── src/    (库源码)
# ├── tests/  (测试源码)
# └── include/  (头文件)

SRC_DIR := src
TEST_DIR := tests
OBJ_DIR := obj
BIN_DIR := bin
LIB_DIR := lib

# --- 3. 源码发现 (Source Discovery) ---

# 查找 src/ 下所有的 .c 文件 (包括子目录, 如 src/std/bump.c)
# $(wildcard ...) 会展开成: src/std/bump.c src/other.c ...
LIB_SRCS := $(shell find $(SRC_DIR) -name '*.c')

# 查找 tests/ 下所有的 .c 文件
# $(wildcard tests/test_bump.c)
TEST_SRCS := $(wildcard $(TEST_DIR)/test_*.c)

# --- 4. 对象文件推导 (Object Derivation) ---

# 目标: src/std/bump.c -> obj/src/std/bump.o
# $(patsubst ...) 是一个模式替换函数
# 它保留了子目录结构，这非常重要！
LIB_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/src/%.o,$(LIB_SRCS))

# 目标: tests/test_bump.c -> obj/tests/test_bump.o
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(OBJ_DIR)/tests/%.o,$(TEST_SRCS))

# 目标: 最终的库文件
LIB_TARGET := $(LIB_DIR)/libfluf.a

# 目标: 最终的测试可执行文件
# tests/test_bump.c -> bin/test_bump
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

# 查找所有头文件，用于依赖跟踪
HEADERS := $(shell find include -name '*.h')

# --- 5. 核心规则 (Recipes) ---

# 默认目标 (当只输入 `make`)
# .PHONY: all 告诉 make 'all' 不是一个真实的文件名
.PHONY: all
all: $(LIB_TARGET)

# 构建库
# $@ -> 目标 (lib/libfluf.a)
# $^ -> 所有的依赖 ($(LIB_OBJS))
$(LIB_TARGET): $(LIB_OBJS)
	@echo "  AR $@"
	@mkdir -p $(dir $@)
	@ar rcs $@ $^

# 模式规则: 如何从 .c 构建 .o (用于库)
# $< -> 第一个依赖 (src/std/bump.c)
$(OBJ_DIR)/src/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "  CC $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# 模式规则: 如何从 .c 构建 .o (用于测试)
$(OBJ_DIR)/tests/%.o: $(TEST_DIR)/%.c $(HEADERS)
	@echo "  CC $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# --- 6. 测试规则 (Testing) ---

# .PHONY: test 告诉 make 'test' 不是一个真实的文件名
.PHONY: test
test: $(TEST_BINS)
	@echo "--- [fluf] Running All Tests ---"
	@$(foreach test,$(TEST_BINS), echo "  RUN $(test)"; ./$(test);)
	@echo "--- [fluf] All Tests Passed ---"

# 模式规则: 如何链接一个测试可执行文件
# bin/test_bump 依赖 obj/tests/test_bump.o 和 lib/libfluf.a
# $< -> 第一个依赖 (obj/tests/test_bump.o)
$(BIN_DIR)/%: $(OBJ_DIR)/tests/%.o $(LIB_TARGET)
	@echo "  LINK $@"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIB_TARGET) $(LDLIBS)


# --- 7. 清理 (Cleaning) ---

.PHONY: clean
clean:
	@echo "  CLEAN"
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR)