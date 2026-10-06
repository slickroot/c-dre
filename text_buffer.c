#include <stdlib.h>
#include <string.h>

#include "text_buffer.h"

void text_buffer_init(struct text_buffer *buf, int cap)
{
	buf->data = malloc(cap + 1);
	buf->len = 0;
	buf->cap = cap;
	buf->cursor = 0;
	buf->data[0] = 0;
}

void text_buffer_free(struct text_buffer *buf)
{
	free(buf->data);
}

int text_buffer_insert(struct text_buffer *buf, char c)
{
	if (buf->len >= buf->cap)
		return 0;

	memmove(&buf->data[buf->cursor + 1], &buf->data[buf->cursor],
		buf->len - buf->cursor);
	buf->data[buf->cursor] = c;
	buf->len++;
	buf->cursor++;
	buf->data[buf->len] = 0;
	return 1;
}

int text_buffer_backspace(struct text_buffer *buf)
{
	if (buf->cursor <= 0)
		return 0;

	memmove(&buf->data[buf->cursor - 1], &buf->data[buf->cursor],
		buf->len - buf->cursor);
	buf->len--;
	buf->cursor--;
	buf->data[buf->len] = 0;
	return 1;
}

int text_buffer_left(struct text_buffer *buf)
{
	if (buf->cursor <= 0)
		return 0;

	buf->cursor--;
	return 1;
}

int text_buffer_right(struct text_buffer *buf)
{
	if (buf->cursor >= buf->len)
		return 0;

	buf->cursor++;
	return 1;
}