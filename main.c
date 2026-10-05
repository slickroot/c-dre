#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static struct termios saved_tty;
static int cols;
static int n_A;
static int n_a;

static void enter(void) {
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  cols = ws.ws_col;

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
}

static void restore(void) {
  write(STDOUT_FILENO, "\x1b[?1049l", sizeof "\x1b[?1049l" - 1);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_tty);
  fflush(stdout);
}

static void paint_square(int r, int g, int b) {
  char sgr[32];
  int len = snprintf(sgr, sizeof sgr, "\x1b[48;2;%d;%d;%dm", r, g, b);
  write(STDOUT_FILENO, sgr, len);
  write(STDOUT_FILENO, "  ", sizeof "  " - 1);
  write(STDOUT_FILENO, "\x1b[0m", sizeof "\x1b[0m" - 1);
}

static void paint_at(int i) {
  char cup[32];
  int len = snprintf(cup, sizeof cup, "\x1b[%d;%dH", 2 * i / cols + 1,
                     2 * i % cols + 1);
  write(STDOUT_FILENO, cup, len);
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
    if (byte == 'a') {
      paint_square(63, 63, 70);
      n_a++;
    }
    if (byte == 'A') {
      paint_at(n_A);
      paint_square(42, 42, 46);
      if (n_a > 0) {
        paint_at(n_A + n_a);
        paint_square(63, 63, 70);
      }
      n_A++;
    }
  }

  return 0;
}