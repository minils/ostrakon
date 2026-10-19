#pragma once
#include <pebble.h>

// Scrollable view of a memo. The phone converts the markdown into lines of
// "<type><text>" (see markdownToLines in index.js), which are drawn with
// fonts and markers per type.
typedef struct MemoView MemoView;

// Called when the view is scrolled close to its start or end while more
// lines can be loaded there
typedef void (*MemoViewEdgeHandler)(bool at_end, void *context);

// click_config_provider adds the window's own buttons; up and down scroll.
// edge_handler may be NULL.
MemoView *memo_view_create(Layer *parent, GRect frame,
                           ClickConfigProvider click_config_provider,
                           Window *window, MemoViewEdgeHandler edge_handler,
                           void *context);
void memo_view_destroy(MemoView *view);

// Shows typed lines and returns the number of blocks, like
// memo_view_add_lines. The buffer is modified and must stay valid.
int memo_view_set_lines(MemoView *view, char *lines);

// Adds a page of typed lines before or after the shown ones, without
// moving the text on screen. Returns the number of blocks added, which
// memo_view_remove_blocks takes to remove the page again. The buffer is
// modified and must stay valid.
int memo_view_add_lines(MemoView *view, char *lines, bool at_end);
void memo_view_remove_blocks(MemoView *view, int count, bool at_end);

// Shows loading dots above or below the lines, where more can be loaded
void memo_view_set_more(MemoView *view, bool above, bool below);

// Animates the loading dots
void memo_view_set_loading(MemoView *view, bool loading);

// Calls the edge handler if the view is close to the given edge and more
// lines can be loaded there, e.g. after a short page was added
void memo_view_check_edge(MemoView *view, bool at_end);

// Shows a plain message, e.g. while loading. The text is not copied.
void memo_view_set_message(MemoView *view, const char *message);
