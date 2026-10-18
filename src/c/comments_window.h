#pragma once
#include <pebble.h>

// Shows the comments of a memo. Select adds a new comment.
void comments_window_push(const char *memo_name);

// Loads the comments again if the window shows those of the given memo
void comments_window_reload(const char *memo_name);
