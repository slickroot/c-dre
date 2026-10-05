#include <assert.h>

#include "input.h"

static void test_quit(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 0x03, MODE_TYPE, 0);
  assert(ev.type == EVENT_QUIT);
}

static void test_printable_char(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'h', MODE_TYPE, 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'h');

  ev = input_parse(&parser, '1', MODE_TYPE, 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == '1');

  ev = input_parse(&parser, ' ', MODE_TYPE, 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == ' ');
}

static void test_summon_band_key(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'a', MODE_TYPE, 0);
  assert(ev.type == EVENT_SUMMON_BAND);
}

static void test_a_is_char_once_band_is_drawn(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'a', MODE_TYPE, 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'a');
}

static void test_backspace_bytes(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 0x7f, MODE_TYPE, 1);
  assert(ev.type == EVENT_BACKSPACE);

  ev = input_parse(&parser, 0x08, MODE_TYPE, 1);
  assert(ev.type == EVENT_BACKSPACE);
}

static void test_behavior_is_the_same_in_move_mode(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'a', MODE_MOVE, 0);
  assert(ev.type == EVENT_SUMMON_BAND);

  ev = input_parse(&parser, 'z', MODE_MOVE, 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'z');

  ev = input_parse(&parser, 0x7f, MODE_MOVE, 1);
  assert(ev.type == EVENT_BACKSPACE);

  ev = input_parse(&parser, 0x03, MODE_MOVE, 1);
  assert(ev.type == EVENT_QUIT);
}

static void test_escape_yields_no_event(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, '\x1b', MODE_TYPE, 1);
  assert(ev.type == EVENT_NONE);

  ev = input_parse(&parser, '[', MODE_TYPE, 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == '[');

  ev = input_parse(&parser, 'D', MODE_TYPE, 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'D');
}

int main(void) {
  test_quit();
  test_printable_char();
  test_summon_band_key();
  test_a_is_char_once_band_is_drawn();
  test_backspace_bytes();
  test_behavior_is_the_same_in_move_mode();
  test_escape_yields_no_event();
  return 0;
}