#include <assert.h>
#include <string.h>

#include "text_buffer.h"

#define TEST_CAP 8

static void test_init(void)
{
	struct text_buffer buf;
	text_buffer_init(&buf, TEST_CAP);

	assert(buf.cap == TEST_CAP);
	assert(buf.len == 0);
	assert(buf.cursor == 0);
	assert(buf.data[0] == 0);

	text_buffer_free(&buf);
}

static void test_insert_at_end(void)
{
	struct text_buffer buf;
	text_buffer_init(&buf, TEST_CAP);

	assert(text_buffer_insert(&buf, 'a') == 1);
	assert(text_buffer_insert(&buf, 'b') == 1);
	assert(text_buffer_insert(&buf, 'c') == 1);

	assert(buf.len == 3);
	assert(buf.cursor == 3);
	assert(strcmp(buf.data, "abc") == 0);

	text_buffer_free(&buf);
}

static void test_insert_at_beginning(void)
{
	struct text_buffer buf;
	text_buffer_init(&buf, TEST_CAP);

	text_buffer_insert(&buf, 'b');
	text_buffer_insert(&buf, 'c');
	text_buffer_left(&buf);
	text_buffer_left(&buf);

	assert(buf.cursor == 0);
	assert(text_buffer_insert(&buf, 'a') == 1);

	assert(buf.len == 3);
	assert(buf.cursor == 1);
	assert(strcmp(buf.data, "abc") == 0);

	text_buffer_free(&buf);
}

static void test_insert_at_middle(void)
{
	struct text_buffer buf;
	text_buffer_init(&buf, TEST_CAP);

	text_buffer_insert(&buf, 'a');
	text_buffer_insert(&buf, 'c');
	text_buffer_left(&buf);

	assert(buf.cursor == 1);
	assert(text_buffer_insert(&buf, 'b') == 1);

	assert(buf.len == 3);
	assert(buf.cursor == 2);
	assert(strcmp(buf.data, "abc") == 0);

	text_buffer_free(&buf);
}

static void test_insert_at_capacity(void)
{
	struct text_buffer buf;
	text_buffer_init(&buf, TEST_CAP);

	char full[TEST_CAP + 1];
	for (int i = 0; i < TEST_CAP; i++) {
		assert(text_buffer_insert(&buf, 'x') == 1);
		full[i] = 'x';
	}
	full[TEST_CAP] = 0;

	assert(buf.len == TEST_CAP);
	assert(buf.cursor == TEST_CAP);
	assert(strcmp(buf.data, full) == 0);

	assert(text_buffer_insert(&buf, 'y') == 0);

	assert(buf.len == TEST_CAP);
	assert(buf.cursor == TEST_CAP);
	assert(strcmp(buf.data, full) == 0);

	text_buffer_free(&buf);
}

static void test_backspace_at_start(void)
{
	struct text_buffer buf;
	text_buffer_init(&buf, TEST_CAP);

	text_buffer_insert(&buf, 'a');
	text_buffer_left(&buf);

	assert(buf.cursor == 0);
	assert(text_buffer_backspace(&buf) == 0);

	assert(buf.len == 1);
	assert(buf.cursor == 0);
	assert(strcmp(buf.data, "a") == 0);

	text_buffer_free(&buf);
}

static void test_backspace_in_middle(void)
{
	struct text_buffer buf;
	text_buffer_init(&buf, TEST_CAP);

	text_buffer_insert(&buf, 'a');
	text_buffer_insert(&buf, 'b');
	text_buffer_insert(&buf, 'c');
	text_buffer_left(&buf);

	assert(buf.cursor == 2);
	assert(text_buffer_backspace(&buf) == 1);

	assert(buf.len == 2);
	assert(buf.cursor == 1);
	assert(strcmp(buf.data, "ac") == 0);

	text_buffer_free(&buf);
}

static void test_backspace_at_end(void)
{
	struct text_buffer buf;
	text_buffer_init(&buf, TEST_CAP);

	text_buffer_insert(&buf, 'a');
	text_buffer_insert(&buf, 'b');
	text_buffer_insert(&buf, 'c');

	assert(buf.cursor == 3);
	assert(text_buffer_backspace(&buf) == 1);

	assert(buf.len == 2);
	assert(buf.cursor == 2);
	assert(strcmp(buf.data, "ab") == 0);

	text_buffer_free(&buf);
}

static void test_move_left_clamps_at_start(void)
{
	struct text_buffer buf;
	text_buffer_init(&buf, TEST_CAP);

	text_buffer_insert(&buf, 'a');
	text_buffer_insert(&buf, 'b');

	assert(text_buffer_left(&buf) == 1);
	assert(buf.cursor == 1);
	assert(text_buffer_left(&buf) == 1);
	assert(buf.cursor == 0);
	assert(text_buffer_left(&buf) == 0);
	assert(buf.cursor == 0);
	assert(buf.len == 2);
	assert(strcmp(buf.data, "ab") == 0);

	text_buffer_free(&buf);
}

static void test_move_right_clamps_at_end(void)
{
	struct text_buffer buf;
	text_buffer_init(&buf, TEST_CAP);

	text_buffer_insert(&buf, 'a');
	text_buffer_insert(&buf, 'b');

	text_buffer_left(&buf);

	assert(buf.cursor == 1);
	assert(text_buffer_right(&buf) == 1);
	assert(buf.cursor == 2);
	assert(text_buffer_right(&buf) == 0);
	assert(buf.cursor == 2);
	assert(buf.len == 2);
	assert(strcmp(buf.data, "ab") == 0);

	text_buffer_free(&buf);
}

int main(void)
{
	test_init();
	test_insert_at_end();
	test_insert_at_beginning();
	test_insert_at_middle();
	test_insert_at_capacity();
	test_backspace_at_start();
	test_backspace_in_middle();
	test_backspace_at_end();
	test_move_left_clamps_at_start();
	test_move_right_clamps_at_end();
	return 0;
}