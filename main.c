#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static struct termios saved_tty;

static int cols;
static char *label;
static int len;
static int cursor;
static int seq;
static int band_drawn;

static void enter(void) {
  tcgetattr(STDIN_FILENO, &saved_tty);

  struct termios raw = saved_tty;
  cfmakeraw(&raw);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

  write(STDOUT_FILENO, "\x1b[?1049h", sizeof "\x1b[?1049h" - 1);
  write(STDOUT_FILENO, "\x1b[?25l", sizeof "\x1b[?25l" - 1);
  write(STDOUT_FILENO, "\x1b[48;2;10;10;11m", sizeof "\x1b[48;2;10;10;11m" - 1);
  write(STDOUT_FILENO, "\x1b[2J", sizeof "\x1b[2J" - 1);
  write(STDOUT_FILENO, "\x1b[1;1H", sizeof "\x1b[1;1H" - 1);

  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  cols = ws.ws_col;

  label = malloc(cols + 1);
  for (int i = 0; i <= cols; i++)
    label[i] = 0;
}

static void restore(void) {
  write(STDOUT_FILENO, "\x1b[?1049l", sizeof "\x1b[?1049l" - 1);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_tty);
  fflush(stdout);
}

static void paint_label(void) {
  if (cols == 0)
    return;
  if (len >= cols)
    return;

  static const char prefix[] = "\x1b[48;2;63;63;70m"
                               "\x1b[38;2;201;201;207m"
                               "\x1b[1;1H"
                               "\x1b[K\r\n"
                               "\x1b[K\r\n"
                               "\x1b[K"
                               "\x1b[?25h";

  char cup[32];
  int n = snprintf(cup, sizeof cup, "\x1b[2;%dH", (cols - len) / 2 + 1);

  write(STDOUT_FILENO, prefix, sizeof prefix - 1);
  write(STDOUT_FILENO, cup, n);
  write(STDOUT_FILENO, label, len);
  write(STDOUT_FILENO, "\x1b[0m", sizeof "\x1b[0m" - 1);

  char caret[32];
  int c = snprintf(caret, sizeof caret, "\x1b[2;%dH", (cols - len) / 2 + cursor + 1);
  write(STDOUT_FILENO, caret, c);
}

int main(void) {
  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "dre: stdin is not a terminal\n");
    exit(1);
  }

  atexit(restore);
  enter();

  char byte;
  while (read(STDIN_FILENO, &byte, 1) == 1) {
    if (byte == 0x03)
      break;
    if (seq == 0) {
      if (byte == 0x1b) {
        seq = 1;
        continue;
      }
      if (!band_drawn && byte == 'a') {
        band_drawn = 1;
        paint_label();
        continue;
      }
      if (byte == 0x7f || byte == 0x08) {
        if (cursor > 0) {
          memmove(&label[cursor - 1], &label[cursor], len - cursor);
          len--;
          cursor--;
          label[len] = 0;
          paint_label();
        }
        continue;
      }
      if (byte >= 0x20 && byte <= 0x7e && len < cols) {
        memmove(&label[cursor + 1], &label[cursor], len - cursor);
        label[cursor] = byte;
        len++;
        cursor++;
        label[len] = 0;
        paint_label();
      }
      continue;
    }

    if (seq == 1 && byte == 0x5b) {
      // `[` is 0x5b, inside the final-byte range, so it must be taken as the
      // introducer before the final-byte rule gets a chance to end the sequence.
      seq = 2;
      continue;
    }

    if (seq == 2 && byte < 0x40) {
      // Parameters are consumed and thrown away; a 0x1b abandons the sequence.
      if (byte == 0x1b)
        seq = 1;
      continue;
    }

    seq = 0;
    if (byte == 'D') {
      if (cursor > 0)
        cursor--;
      paint_label();
    }
    if (byte == 'C') {
      if (cursor < len)
        cursor++;
      paint_label();
    }
  }

  return 0;
}