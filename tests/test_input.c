#include <assert.h>

#include "input.h"

static void test_quit(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 0x03, 0);
  assert(ev.type == EVENT_QUIT);
}

static void test_printable_char(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'h', 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'h');

  ev = input_parse(&parser, '1', 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == '1');

  ev = input_parse(&parser, ' ', 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == ' ');
}

static void test_summon_band_key(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'a', 0);
  assert(ev.type == EVENT_SUMMON_BAND);
}

static void test_a_is_char_once_band_is_drawn(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 'a', 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'a');
}

static void test_backspace_bytes(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, 0x7f, 1);
  assert(ev.type == EVENT_BACKSPACE);

  ev = input_parse(&parser, 0x08, 1);
  assert(ev.type == EVENT_BACKSPACE);
}

static void test_arrow_left(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, '\x1b', 1);
  assert(ev.type == EVENT_NONE);
  ev = input_parse(&parser, '[', 1);
  assert(ev.type == EVENT_NONE);
  ev = input_parse(&parser, 'D', 1);
  assert(ev.type == EVENT_LEFT);
}

static void test_arrow_right(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, '\x1b', 1);
  assert(ev.type == EVENT_NONE);
  ev = input_parse(&parser, '[', 1);
  assert(ev.type == EVENT_NONE);
  ev = input_parse(&parser, 'C', 1);
  assert(ev.type == EVENT_RIGHT);
}

static void test_discards_unsupported_sequences(void) {
  struct input_parser parser;
  input_parser_init(&parser);

  struct key_event ev = input_parse(&parser, '\x1b', 1);
  assert(ev.type == EVENT_NONE);
  ev = input_parse(&parser, '[', 1);
  assert(ev.type == EVENT_NONE);
  ev = input_parse(&parser, '3', 1);
  assert(ev.type == EVENT_NONE);
  ev = input_parse(&parser, '~', 1);
  assert(ev.type == EVENT_NONE);

  ev = input_parse(&parser, '\x1b', 1);
  assert(ev.type == EVENT_NONE);
  ev = input_parse(&parser, '[', 1);
  assert(ev.type == EVENT_NONE);
  ev = input_parse(&parser, 'H', 1);
  assert(ev.type == EVENT_NONE);

  ev = input_parse(&parser, 'k', 1);
  assert(ev.type == EVENT_CHAR);
  assert(ev.ch == 'k');
}

int main(void) {
  test_quit();
  test_printable_char();
  test_summon_band_key();
  test_a_is_char_once_band_is_drawn();
  test_backspace_bytes();
  test_arrow_left();
  test_arrow_right();
  test_discards_unsupported_sequences();
  return 0;
}
