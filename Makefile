CC = gcc
CFLAGS = -Wall -Wextra
DEBUG_FLAGS = -DDEBUG -g

LIB_DIR = lib
TEST_DIR = tests

# Get all source files in the lib folder
LIB_SRCS = $(wildcard $(LIB_DIR)/*.c)
# Generate object file names for the lib sources
LIB_OBJS = $(patsubst %.c, %.o, $(LIB_SRCS))

# Get all source files in the tests folder
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
# Generate executable names for each test file
TEST_TARGETS = $(patsubst %.c, %, $(TEST_SRCS))

# Target for building all tests
all: $(TEST_TARGETS)

# Rule for building each test file
$(TEST_TARGETS): %: $(LIB_OBJS) $(TEST_DIR)/%.c
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $(LIB_OBJS) $(filter-out $(LIB_DIR)/ppos_core.c, $(LIB_SRCS)) $(filter-out $(LIB_DIR)/ppos_data.h, $(wildcard $(LIB_DIR)/*.h)) $^

%.o: %.c
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

clean:
	rm -f $(TEST_TARGETS) $(LIB_OBJS)
