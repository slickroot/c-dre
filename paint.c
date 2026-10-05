#include <stdio.h>
#include <unistd.h>

#include "paint.h"

void paint_wallpaper(void) {
  write(STDOUT_FILENO, "\x1b[?1049h", sizeof "\x1b[?1049h" - 1);
  write(STDOUT_FILENO, "\x1b[?25l", sizeof "\x1b[?25l" - 1);
  write(STDOUT_FILENO, "\x1b[48;2;10;10;11m", sizeof "\x1b[48;2;10;10;11m" - 1);
  write(STDOUT_FILENO, "\x1b[2J", sizeof "\x1b[2J" - 1);
  write(STDOUT_FILENO, "\x1b[1;1H", sizeof "\x1b[1;1H" - 1);
}

void paint_label(const struct text_buffer *buf, int cols, int row) {
  if (cols == 0)
    return;
  if (buf->len >= cols)
    return;

  static const char ink[] = "\x1b[38;2;201;201;207m";
  static const char erases[] = "\x1b[K"
                               "\x1b[?25h";

  char top[32];
  int t = snprintf(top, sizeof top, "\x1b[%d;1H", row);

  write(STDOUT_FILENO, ink, sizeof ink - 1);
  write(STDOUT_FILENO, top, t);
  write(STDOUT_FILENO, erases, sizeof erases - 1);

  char cup[32];
  int n = snprintf(cup, sizeof cup, "\x1b[%d;%dH", row, (cols - buf->len) / 2 + 1);

  write(STDOUT_FILENO, cup, n);
  write(STDOUT_FILENO, buf->data, buf->len);

  char caret[32];
  int c = snprintf(caret, sizeof caret, "\x1b[%d;%dH", row,
                   (cols - buf->len) / 2 + buf->cursor + 1);
  write(STDOUT_FILENO, caret, c);
}