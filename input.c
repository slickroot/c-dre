#include "input.h"

void input_parser_init(struct input_parser *parser) {
  parser->seq = 0;
}

struct key_event input_parse(struct input_parser *parser, char byte, int band_drawn) {
  struct key_event ev = { EVENT_NONE, 0 };

  if (byte == 0x03)
    return (struct key_event){ EVENT_QUIT, 0 };

  if (parser->seq == 0 && byte == 0x1b) {
    parser->seq = 1;
    return ev;
  }

  if (parser->seq == 1) {
    if (byte == 0x5b) {
      parser->seq = 2;
      return ev;
    }
    parser->seq = 0;
    return ev;
  }

  if (parser->seq == 2) {
    if (byte < 0x40) {
      if (byte == 0x1b)
        parser->seq = 1;
      return ev;
    }
    parser->seq = 0;
    if (byte == 'D')
      return (struct key_event){ EVENT_LEFT, 0 };
    if (byte == 'C')
      return (struct key_event){ EVENT_RIGHT, 0 };
    return ev;
  }

  if (!band_drawn && byte == 'a')
    return (struct key_event){ EVENT_SUMMON_BAND, 0 };

  if (byte == 0x7f || byte == 0x08)
    return (struct key_event){ EVENT_BACKSPACE, 0 };

  if (byte >= 0x20 && byte <= 0x7e)
    return (struct key_event){ EVENT_CHAR, byte };

  return ev;
}