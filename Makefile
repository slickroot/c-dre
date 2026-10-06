CC = cc
CFLAGS = -std=c11 -Wall -Wextra -O2
CPPFLAGS = -I.

BUILD_DIR = build
SRCS = main.c text_buffer.c input.c paint.c
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
TEST_BINS = $(BUILD_DIR)/test_text_buffer $(BUILD_DIR)/test_input

all: dre

dre: $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

test: $(TEST_BINS)
	$(BUILD_DIR)/test_text_buffer
	$(BUILD_DIR)/test_input

$(BUILD_DIR)/test_text_buffer: tests/test_text_buffer.c text_buffer.c text_buffer.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_text_buffer.c text_buffer.c

$(BUILD_DIR)/test_input: tests/test_input.c input.c input.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_input.c input.c

clean:
	rm -rf $(BUILD_DIR) dre

FORMAT_FILES = $(wildcard *.c *.h tests/*.c tests/*.h)

format:
	clang-format -i $(FORMAT_FILES)

lint:
	clang-format --dry-run --Werror $(FORMAT_FILES)

.PHONY: all test clean format lint