#ifndef TEXT_BUFFER_H
#define TEXT_BUFFER_H

struct text_buffer {
  char *data;
  int len;
  int cap;
  int cursor;
};

void text_buffer_init(struct text_buffer *buf, int cap);
void text_buffer_free(struct text_buffer *buf);
int text_buffer_insert(struct text_buffer *buf, char c);
int text_buffer_backspace(struct text_buffer *buf);
int text_buffer_left(struct text_buffer *buf);
int text_buffer_right(struct text_buffer *buf);

#endif