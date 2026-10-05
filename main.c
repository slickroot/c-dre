#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "input.h"
#include "paint.h"
#include "text_buffer.h"

static struct termios saved_tty;

static int cols;

static void enter(void) {
  tcgetattr(STDIN_FILENO, &saved_tty);

  struct termios raw = saved_tty;
  cfmakeraw(&raw);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  paint_wallpaper();

  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  cols = ws.ws_col;
}

static void restore(void) {
  write(STDOUT_FILENO, "\x1b[?1049l", sizeof "\x1b[?1049l" - 1);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_tty);
  fflush(stdout);
}

int main(void) {
  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "dre: stdin is not a terminal\n");
    exit(1);
  }

  atexit(restore);
  enter();

  struct text_buffer buf;
  text_buffer_init(&buf, cols);

  struct input_parser parser;
  input_parser_init(&parser);

  enum app_mode mode = MODE_MOVE;

  char byte;
  while (read(STDIN_FILENO, &byte, 1) == 1) {
    struct key_event ev = input_parse(&parser, byte, mode);
    int changed = 0;

    switch (ev.type) {
    case EVENT_QUIT:
      goto done;
    case EVENT_ADD_BAND:
      mode = MODE_TYPE;
      paint_label(&buf, cols);
      break;
    case EVENT_ESCAPE:
      mode = MODE_MOVE;
      break;
    case EVENT_ENTER_TYPE:
      mode = MODE_TYPE;
      break;
    case EVENT_CHAR:
      changed = text_buffer_insert(&buf, ev.ch);
      break;
    case EVENT_BACKSPACE:
      changed = text_buffer_backspace(&buf);
      break;
    case EVENT_NONE:
      break;
    }

    if (changed)
      paint_label(&buf, cols);
  }

done:
  text_buffer_free(&buf);

  return 0;
}
