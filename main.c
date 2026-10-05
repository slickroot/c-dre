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

struct band {
  struct text_buffer buf;
  int top_row;
  struct band *next;
  struct band *prev;
};

static struct band *bands;
static struct band *selected;

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
  write(STDOUT_FILENO, "\x1b[0m", sizeof "\x1b[0m" - 1);
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
    case EVENT_ADD_BAND: {
      struct band *b = malloc(sizeof *b);
      if (b) {
        b->top_row = bands ? bands->top_row + 3 : 1;
        text_buffer_init(&b->buf, cols);
        b->next = bands;
        b->prev = NULL;
        if (bands)
          bands->prev = b;
        bands = b;
        selected = b;
        mode = MODE_TYPE;
        paint_label(&b->buf, cols, b->top_row);
      }
      break;
    }
    case EVENT_ESCAPE:
      mode = MODE_MOVE;
      break;
    case EVENT_ENTER_TYPE:
      mode = MODE_TYPE;
      break;
    case EVENT_SELECT_UP:
      if (selected && selected->next)
        selected = selected->next;
      break;
    case EVENT_SELECT_DOWN:
      if (selected && selected->prev)
        selected = selected->prev;
      break;
    case EVENT_CHAR:
      if (selected == NULL)
        continue;
      changed = text_buffer_insert(&selected->buf, ev.ch);
      break;
    case EVENT_BACKSPACE:
      if (selected == NULL)
        continue;
      changed = text_buffer_backspace(&selected->buf);
      break;
    case EVENT_NONE:
      break;
    }

    if (changed || ev.type == EVENT_SELECT_UP ||
        ev.type == EVENT_SELECT_DOWN)
      paint_label(&selected->buf, cols, selected->top_row);
  }

done:
  while (bands) {
    struct band *next = bands->next;
    text_buffer_free(&bands->buf);
    free(bands);
    bands = next;
  }

  return 0;
}
