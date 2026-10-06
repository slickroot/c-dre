#include <assert.h>

#include "input.h"

static void test_quit(void)
{
	struct key_event ev = input_parse(0x03, MODE_MOVE);
	assert(ev.type == EVENT_QUIT);

	ev = input_parse(0x03, MODE_TYPE);
	assert(ev.type == EVENT_QUIT);
}

static void test_printable_char(void)
{
	struct key_event ev = input_parse('h', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == 'h');

	ev = input_parse('1', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == '1');

	ev = input_parse(' ', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == ' ');
}

static void test_escape_in_type_mode(void)
{
	struct key_event ev = input_parse('\x1b', MODE_TYPE);
	assert(ev.type == EVENT_ESCAPE);

	ev = input_parse('[', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == '[');

	ev = input_parse('D', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == 'D');
}

static void test_escape_in_move_mode_is_none(void)
{
	struct key_event ev = input_parse('\x1b', MODE_MOVE);
	assert(ev.type == EVENT_NONE);
}

static void test_add_band_key(void)
{
	struct key_event ev = input_parse('a', MODE_MOVE);
	assert(ev.type == EVENT_ADD_BAND);
}

static void test_a_is_char_in_type_mode(void)
{
	struct key_event ev = input_parse('a', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == 'a');
}

static void test_i_enters_type_mode(void)
{
	struct key_event ev = input_parse('i', MODE_MOVE);
	assert(ev.type == EVENT_ENTER_TYPE);
}

static void test_i_is_char_in_type_mode(void)
{
	struct key_event ev = input_parse('i', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == 'i');
}

static void test_k_and_j_in_move_mode(void)
{
	struct key_event ev = input_parse('k', MODE_MOVE);
	assert(ev.type == EVENT_SELECT_UP);

	ev = input_parse('j', MODE_MOVE);
	assert(ev.type == EVENT_SELECT_DOWN);
}

static void test_k_and_j_are_char_in_type_mode(void)
{
	struct key_event ev = input_parse('k', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == 'k');

	ev = input_parse('j', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == 'j');
}

static void test_delete_band_key(void)
{
	struct key_event ev = input_parse('d', MODE_MOVE);
	assert(ev.type == EVENT_DELETE_BAND);
}

static void test_d_is_char_in_type_mode(void)
{
	struct key_event ev = input_parse('d', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == 'd');
}

static void test_toggle_dim_key(void)
{
	struct key_event ev = input_parse('-', MODE_MOVE);
	assert(ev.type == EVENT_TOGGLE_DIM);
}

static void test_dash_is_char_in_type_mode(void)
{
	struct key_event ev = input_parse('-', MODE_TYPE);
	assert(ev.type == EVENT_CHAR);
	assert(ev.ch == '-');
}

static void test_move_mode_ignores_typing_keys(void)
{
	struct key_event ev = input_parse('x', MODE_MOVE);
	assert(ev.type == EVENT_NONE);

	ev = input_parse('b', MODE_MOVE);
	assert(ev.type == EVENT_NONE);

	ev = input_parse(' ', MODE_MOVE);
	assert(ev.type == EVENT_NONE);

	ev = input_parse('z', MODE_MOVE);
	assert(ev.type == EVENT_NONE);

	ev = input_parse('1', MODE_MOVE);
	assert(ev.type == EVENT_NONE);
}

static void test_backspace_bytes(void)
{
	struct key_event ev = input_parse(0x7f, MODE_TYPE);
	assert(ev.type == EVENT_BACKSPACE);

	ev = input_parse(0x08, MODE_TYPE);
	assert(ev.type == EVENT_BACKSPACE);
}

static void test_backspace_is_none_in_move_mode(void)
{
	struct key_event ev = input_parse(0x7f, MODE_MOVE);
	assert(ev.type == EVENT_NONE);

	ev = input_parse(0x08, MODE_MOVE);
	assert(ev.type == EVENT_NONE);
}

int main(void)
{
	test_quit();
	test_printable_char();
	test_escape_in_type_mode();
	test_escape_in_move_mode_is_none();
	test_add_band_key();
	test_a_is_char_in_type_mode();
	test_i_enters_type_mode();
	test_i_is_char_in_type_mode();
	test_k_and_j_in_move_mode();
	test_k_and_j_are_char_in_type_mode();
	test_delete_band_key();
	test_d_is_char_in_type_mode();
	test_toggle_dim_key();
	test_dash_is_char_in_type_mode();
	test_move_mode_ignores_typing_keys();
	test_backspace_bytes();
	test_backspace_is_none_in_move_mode();
	return 0;
}
