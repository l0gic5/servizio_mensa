SRC_DIR = src
TEST_DIR = test
BIN_DIR = bin
BUILD_DIR = build
UNITY_DIR = unity
INCLUDE_DIR = include
DOCS_DIR = docs/html

CC = gcc

# -Wpedantic
CFLAGS = -std=c99 -g -Wall -Wextra -Wconversion -Wvla -Werror -D_GNU_SOURCE -pthread

UNITY_CONF = -DUNITY_INCLUDE_DOUBLE

INCLUDES = -I$(INCLUDE_DIR) -I$(UNITY_DIR)

COMMON_SRCS = $(wildcard $(SRC_DIR)/common/*.c)
COMMON_OBJS = $(patsubst $(SRC_DIR)/common/%.c, $(BUILD_DIR)/common/%.o, $(COMMON_SRCS))

PROCESS_SRCS = $(wildcard $(SRC_DIR)/processes/*.c)
PROCESS_OBJS = $(patsubst $(SRC_DIR)/processes/%.c, $(BUILD_DIR)/processes/%.o, $(PROCESS_SRCS))
PROCESS_BINS = $(patsubst $(SRC_DIR)/processes/%.c, $(BIN_DIR)/processes/%, $(PROCESS_SRCS))

EXECUTABLES_SRCS = $(wildcard $(SRC_DIR)/executables/*.c)
EXECUTABLES_OBJS = $(patsubst $(SRC_DIR)/executables/%.c, $(BUILD_DIR)/executables/%.o, $(EXECUTABLES_SRCS))
EXECUTABLES_BINS = $(patsubst $(SRC_DIR)/executables/%.c, $(BIN_DIR)/executables/%, $(EXECUTABLES_SRCS))

TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS = $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/test/%.o, $(TEST_SRCS))
TEST_BINS = $(patsubst $(TEST_DIR)/%.c, $(BIN_DIR)/test/%, $(TEST_SRCS))

UNITY_OBJ = $(BUILD_DIR)/unity.o

DOXYFILE = Doxyfile

.PHONY: all test clean docs

all: $(PROCESS_BINS) $(EXECUTABLES_BINS) $(TEST_BINS)

$(PROCESS_BINS): $(BIN_DIR)/processes/%: $(BUILD_DIR)/processes/%.o $(COMMON_OBJS)
	@mkdir -p $(BIN_DIR)/processes
	@echo "Linking Process: $@"
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(EXECUTABLES_BINS): $(BIN_DIR)/executables/%: $(BUILD_DIR)/executables/%.o $(COMMON_OBJS)
	@mkdir -p $(BIN_DIR)/executables
	@echo "Linking Executables tools: $@"
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(TEST_BINS): $(BIN_DIR)/test/%: $(BUILD_DIR)/test/%.o $(COMMON_OBJS) $(UNITY_OBJ)
	@mkdir -p $(BIN_DIR)/test
	@echo "Linking Test: $@"
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(BUILD_DIR)/common/%.o: $(SRC_DIR)/common/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/processes/%.o: $(SRC_DIR)/processes/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/executables/%.o: $(SRC_DIR)/executables/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(UNITY_CONF) $(INCLUDES) -c $< -o $@

$(UNITY_OBJ): $(UNITY_DIR)/unity.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(UNITY_CONF) $(INCLUDES) -c $< -o $@

test: $(TEST_BINS)
	@echo "* Running tests..."
	@for test in $(TEST_BINS); do \
		echo ">> Running $$test"; \
		./$$test; \
		echo ""; \
	done

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BIN_DIR) $(BUILD_DIR) $(DOCS_DIR)

docs:
	@echo "* Generating Doxygen documentation"
	@doxygen $(DOXYFILE)
	@echo "* Generated documentation: $(DOCS_DIR)/index.html"