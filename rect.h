#ifndef RECT_H
#define RECT_H

struct rect {
	int row, col;	/* top-left, 1-based like the terminal */
	int rows, cols; /* height, width */
};

#endif
