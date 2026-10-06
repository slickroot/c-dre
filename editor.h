#ifndef EDITOR_H
#define EDITOR_H

#include "input.h"
#include "layout.h"

struct editor;

struct editor *editor_new(int cols, int rows);
void editor_free(struct editor *e);
void editor_apply(struct editor *e, struct key_event ev);
enum app_mode editor_mode(const struct editor *e);
struct layout layout(const struct editor *e);

#endif