SRC_DIR = src
TEST_DIR = test
BIN_DIR = bin
BUILD_DIR = build
UNITY_DIR = unity

CC = gcc
UNITY_CONF = -DUNITY_INCLUDE_DOUBLE

CFLAGS = -g -Wall -Wextra -Wpedantic -Wconversion $(UNITY_CONF)

INCLUDES = -I$(SRC_DIR) -I$(UNITY_DIR)

# (search corresponding)
SRCS = $(wildcard $(SRC_DIR)/*.c)

# (pattern substitution)
LIB_OBJS = $(patsubst $(SRC_DIR)/main.c, , $(SRCS))
LIB_OBJS := $(patsubst $(SRCS), $(BUILD_DIR)/%.o, $(LIB_OBJS))

MAIN_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

TEST_SRC = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SRC))
UNITY_OBJ = $(BUILD_DIR)/unity.o

MAIN_EXEC = $(BIN_DIR)/main_ex2
TEST_EXEC = $(BIN_DIR)/test_ex2

RUN_PARAMS = $(TEST_DIR)/dataset/iliade.txt 10

DOXYFILE = Doxyfile
DOCS_DIR = docs/html

all: $(MAIN_EXEC) $(TEST_EXEC)

$(MAIN_EXEC): $(MAIN_OBJS)
	@mkdir -p $(BIN_DIR)
	@echo "Linking $(MAIN_EXEC)..."
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(TEST_EXEC): $(TEST_OBJS) $(LIB_OBJS) $(UNITY_OBJ)
	@mkdir -p $(BIN_DIR)
	@echo "Linking $(TEST_EXEC)..."
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(UNITY_OBJ): $(UNITY_DIR)/unity.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@


run: $(MAIN_EXEC)
	@./$(MAIN_EXEC) $(RUN_PARAMS)

test: $(TEST_EXEC)
	@echo "* Running tests..."
	@./$(TEST_EXEC) || true

clean:
	@rm -rf $(BIN_DIR) $(BUILD_DIR) benchmark_temp_output.bin

docs:
	@echo "* generating Doxygen documentation"
	@doxygen $(DOXYFILE)
	@echo "* generated documentation: $(DOCS_DIR)/index.html"

.PHONY: all test clean docs run benchmark