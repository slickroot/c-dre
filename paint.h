#ifndef PAINT_H
#define PAINT_H

#include "text_buffer.h"

void paint_wallpaper(void);
void paint_label(const struct text_buffer *buf, int cols, int row);
void paint_delete_row(int row);
void paint_hide_cursor(void);

#endif