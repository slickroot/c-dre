CC = cc
CFLAGS = -std=c11 -Wall -Wextra -O2
CPPFLAGS = -I.

BUILD_DIR = build
SRCS = main.c editor.c node.c text_buffer.c layout.c display.c input.c paint.c grid.c term.c
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
TEST_BINS = $(BUILD_DIR)/test_text_buffer $(BUILD_DIR)/test_input $(BUILD_DIR)/test_editor $(BUILD_DIR)/test_grid $(BUILD_DIR)/test_paint $(BUILD_DIR)/test_node

all: dre

dre: $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

test: $(TEST_BINS)
	$(BUILD_DIR)/test_text_buffer
	$(BUILD_DIR)/test_input
	$(BUILD_DIR)/test_editor
	$(BUILD_DIR)/test_grid
	$(BUILD_DIR)/test_paint
	$(BUILD_DIR)/test_node

$(BUILD_DIR)/test_text_buffer: tests/test_text_buffer.c text_buffer.c text_buffer.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_text_buffer.c text_buffer.c

$(BUILD_DIR)/test_input: tests/test_input.c input.c input.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_input.c input.c

$(BUILD_DIR)/test_editor: tests/test_editor.c editor.c editor.h node.c node.h rect.h text_buffer.c text_buffer.h layout.c layout.h display.c display.h paint.c paint.h grid.c grid.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_editor.c editor.c node.c text_buffer.c layout.c display.c paint.c grid.c

$(BUILD_DIR)/test_grid: tests/test_grid.c grid.c grid.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_grid.c grid.c

$(BUILD_DIR)/test_paint: tests/test_paint.c paint.c paint.h grid.c grid.h display.h rect.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_paint.c paint.c grid.c

$(BUILD_DIR)/test_node: tests/test_node.c node.c node.h text_buffer.c text_buffer.h rect.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_node.c node.c text_buffer.c

clean:
	rm -rf $(BUILD_DIR) dre

FORMAT_FILES = $(wildcard *.c *.h tests/*.c tests/*.h)

format:
	clang-format -i $(FORMAT_FILES)

lint:
	clang-format --dry-run --Werror $(FORMAT_FILES)

.PHONY: all test clean format lint