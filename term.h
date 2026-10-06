#ifndef TERM_H
#define TERM_H

#include "grid.h"

void term_enter(void);
void term_flush(const struct grid *g);
void term_leave(void);

#endif
