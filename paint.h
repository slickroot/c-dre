#ifndef PAINT_H
#define PAINT_H

#include "display.h"
#include "grid.h"

#define BORDER_CORNER '+'
#define BORDER_HORIZONTAL '-'
#define BORDER_VERTICAL '|'

void paint_frame(const struct display_list *dl, struct grid *g);

#endif
