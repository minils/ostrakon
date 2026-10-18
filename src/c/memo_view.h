#pragma once
#include <pebble.h>

// Scrollable view of a memo. The phone converts the markdown into lines of
// "<type><text>" (see markdownToLines in index.js), which are drawn with
// fonts and markers per type.
typedef struct MemoView MemoView;

// click_config_provider adds the window's own buttons; up and down scroll
MemoView *memo_view_create(Layer *parent, GRect frame,
                           ClickConfigProvider click_config_provider,
                           Window *window);
void memo_view_destroy(MemoView *view);

// Shows typed lines. The buffer is modified and must stay valid.
void memo_view_set_lines(MemoView *view, char *lines);

// Shows a plain message, e.g. while loading. The text is not copied.
void memo_view_set_message(MemoView *view, const char *message);
