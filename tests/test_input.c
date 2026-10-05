#include <assert.h>

#include "input.h"

static void test_quit(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 0x03, MODE_MOVE);
  assert(ev.type == EVENT_QUIT);

  ev = input_parse(&parser, 0x03, MODE_TYPE);
  assert(ev.type == EVENT_QUIT);
}

static void test_printable_char(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'h', MODE_TYPE);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'h');

  ev = input_parse(&parser, '1', MODE_TYPE);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == '1');

  ev = input_parse(&parser, ' ', MODE_TYPE);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == ' ');
}

static void test_escape_in_type_mode(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, '\x1b', MODE_TYPE);
  assert(ev.type == EVENT_ESCAPE);

  ev = input_parse(&parser, '[', MODE_TYPE);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == '[');

  ev = input_parse(&parser, 'D', MODE_TYPE);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'D');
}

static void test_escape_in_move_mode_is_none(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, '\x1b', MODE_MOVE);
  assert(ev.type == EVENT_NONE);
}

static void test_add_band_key(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'a', MODE_MOVE);
  assert(ev.type == EVENT_ADD_BAND);
}

static void test_a_is_char_in_type_mode(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'a', MODE_TYPE);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'a');
}

static void test_i_enters_type_mode(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'i', MODE_MOVE);
  assert(ev.type == EVENT_ENTER_TYPE);
}

static void test_i_is_char_in_type_mode(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'i', MODE_TYPE);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'i');
}

static void test_move_mode_ignores_typing_keys(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'x', MODE_MOVE);
  assert(ev.type == EVENT_NONE);

  ev = input_parse(&parser, 'b', MODE_MOVE);
  assert(ev.type == EVENT_NONE);

  ev = input_parse(&parser, ' ', MODE_MOVE);
  assert(ev.type == EVENT_NONE);

  ev = input_parse(&parser, 'z', MODE_MOVE);
  assert(ev.type == EVENT_NONE);

  ev = input_parse(&parser, '1', MODE_MOVE);
  assert(ev.type == EVENT_NONE);
}

static void test_backspace_bytes(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 0x7f, MODE_TYPE);
  assert(ev.type == EVENT_BACKSPACE);

  ev = input_parse(&parser, 0x08, MODE_TYPE);
  assert(ev.type == EVENT_BACKSPACE);
}

static void test_backspace_is_none_in_move_mode(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 0x7f, MODE_MOVE);
  assert(ev.type == EVENT_NONE);

  ev = input_parse(&parser, 0x08, MODE_MOVE);
  assert(ev.type == EVENT_NONE);
}

int main(void) {
  test_quit();
  test_printable_char();
  test_escape_in_type_mode();
  test_escape_in_move_mode_is_none();
  test_add_band_key();
  test_a_is_char_in_type_mode();
  test_i_enters_type_mode();
  test_i_is_char_in_type_mode();
  test_move_mode_ignores_typing_keys();
  test_backspace_bytes();
  test_backspace_is_none_in_move_mode();
  return 0;
}
