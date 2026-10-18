#pragma once
#include <pebble.h>

// Lists the comments of a memo with author and date. Select opens the
// actions of a comment.
void comments_window_push(const char *memo_name);

// Loads the comments again if the window shows those of the given memo
void comments_window_reload(const char *memo_name);

// Removes a comment from the list, e.g. after it was archived
void comments_window_remove(const char *comment_name);
