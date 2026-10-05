#ifndef INPUT_H
#define INPUT_H

enum key_event_type {
  EVENT_NONE = 0,
  EVENT_QUIT,
  EVENT_SUMMON_BAND,
  EVENT_CHAR,
  EVENT_LEFT,
  EVENT_RIGHT,
  EVENT_BACKSPACE
};

struct key_event {
  enum key_event_type type;
  char ch;
};

struct input_parser {
  int seq;
};

void input_parser_init(struct input_parser *parser);
struct key_event input_parse(struct input_parser *parser, char byte, int band_drawn);

#endif