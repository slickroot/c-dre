#include "input.h"

void input_parser_init(struct input_parser *parser) {
  (void)parser;
}

struct key_event input_parse(struct input_parser *parser, char byte, enum app_mode mode) {
  (void)parser;

  if (byte == 0x03)
    return (struct key_event){ EVENT_QUIT, 0 };

  if (mode == MODE_MOVE) {
    if (byte == 'a')
      return (struct key_event){ EVENT_ADD_BAND, 0 };

    if (byte == 'i')
      return (struct key_event){ EVENT_ENTER_TYPE, 0 };

    if (byte == 'k')
      return (struct key_event){ EVENT_SELECT_UP, 0 };

    if (byte == 'j')
      return (struct key_event){ EVENT_SELECT_DOWN, 0 };

    if (byte == 'd')
      return (struct key_event){ EVENT_DELETE_BAND, 0 };

    return (struct key_event){ EVENT_NONE, 0 };
  }

  if (byte == 0x1b)
    return (struct key_event){ EVENT_ESCAPE, 0 };

  if (byte == 0x7f || byte == 0x08)
    return (struct key_event){ EVENT_BACKSPACE, 0 };

  if (byte >= 0x20 && byte <= 0x7e)
    return (struct key_event){ EVENT_CHAR, byte };

  return (struct key_event){ EVENT_NONE, 0 };
}