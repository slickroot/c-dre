#ifndef PAINT_H
#define PAINT_H

#include "text_buffer.h"

void paint_wallpaper(void);
void paint_label(const struct text_buffer *buf, int cols);

#endif