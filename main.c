#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#ifndef CLOCK_MONO
#define CLOCK_MONO CLOCK_MONOTONIC
#endif

static struct termios saved_tty;
static int cols;
static int rows;
static int n_A;
static int n_a;

static void enter(void) {
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  cols = ws.ws_col;
  rows = ws.ws_row;

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

static void press(char key) {
  if (key == 'a') {
    paint_square(63, 63, 70);
    n_a++;
  }
  if (key == 'A') {
    paint_at(n_A);
    paint_square(42, 42, 46);
    if (n_a > 0) {
      paint_at(n_A + n_a);
      paint_square(63, 63, 70);
    }
    n_A++;
  }
}

static int cmp_ll(const void *l, const void *r) {
  long long a = *(const long long *)l;
  long long b = *(const long long *)r;
  if (a < b)
    return -1;
  if (a > b)
    return 1;
  return 0;
}

__attribute__((unused)) static long long now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONO, &ts);
  return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

__attribute__((unused)) static long long median(long long *v, long n) {
  qsort(v, (size_t)n, sizeof *v, cmp_ll);
  if (n % 2 == 1)
    return v[n / 2];
  return (v[n / 2 - 1] + v[n / 2]) / 2;
}

static void run_bench(int sync) {
  (void)sync;
}

int main(int argc, char **argv) {
  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "dre: stdin is not a terminal\n");
    exit(1);
  }

  int bench = 0;
  int sync = 0;

  if (argc > 2) {
    fprintf(stderr, "usage: dre [--bench|--bench=nosync]\n");
    exit(2);
  }

  if (argc == 2) {
    if (strcmp(argv[1], "--bench") == 0) {
      bench = 1;
      sync = 1;
    } else if (strcmp(argv[1], "--bench=nosync") == 0) {
      bench = 1;
      sync = 0;
    } else {
      fprintf(stderr, "usage: dre [--bench|--bench=nosync]\n");
      exit(2);
    }
  }

  atexit(restore);
  enter();

  if (bench) {
    run_bench(sync);
    return 0;
  }

  char byte;
  while (read(STDIN_FILENO, &byte, 1) == 1) {
    if (byte == 0x03)
      break;
    press(byte);
  }

  return 0;
}
