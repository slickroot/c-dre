#ifndef INPUT_H
#define INPUT_H

enum key_event_type {
	EVENT_NONE = 0,
	EVENT_QUIT,
	EVENT_ADD_BAND,
	EVENT_CHAR,
	EVENT_BACKSPACE,
	EVENT_ESCAPE,
	EVENT_ENTER_TYPE,
	EVENT_SELECT_UP,
	EVENT_SELECT_DOWN,
	EVENT_DELETE_BAND,
	EVENT_TOGGLE_DIM
};

enum app_mode { MODE_MOVE, MODE_TYPE };

struct key_event {
	enum key_event_type type;
	char ch;
};

struct key_event input_parse(char byte, enum app_mode mode);

#endif