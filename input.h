#ifndef INPUT_H
#define INPUT_H

enum key_event_type {
  EVENT_NONE = 0,
  EVENT_QUIT,
  EVENT_SUMMON_BAND,
  EVENT_CHAR,
  EVENT_BACKSPACE
};

enum app_mode {
  MODE_MOVE,
  MODE_TYPE
};

struct key_event {
  enum key_event_type type;
  char ch;
};

struct input_parser {
  int unused;
};

void input_parser_init(struct input_parser *parser);
struct key_event input_parse(struct input_parser *parser, char byte, enum app_mode mode, int band_drawn);

#endif