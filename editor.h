#ifndef EDITOR_H
#define EDITOR_H

#include "input.h"

struct editor;
struct node;

struct editor *editor_new(int cols, int rows);
void editor_free(struct editor *e);
void editor_apply(struct editor *e, struct key_event ev);
enum app_mode editor_mode(const struct editor *e);
struct node *editor_root(const struct editor *e);
const struct node *editor_selected(const struct editor *e);

#endif
