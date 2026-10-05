#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

static struct termios saved_tty;

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

  char byte;
  while (read(STDIN_FILENO, &byte, 1) == 1) {
    if (byte == 0x03)
      break;
  }

  return 0;
}