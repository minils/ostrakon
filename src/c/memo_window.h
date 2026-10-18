#pragma once
#include <pebble.h>

// Shows the full text of a memo, loaded from the phone
void memo_window_push(const char *memo_name);

// Shows a comment of the memo memo_name, with its author and date
void memo_window_push_comment(const char *comment_name, const char *memo_name);

// Loads the memo or comment again if a window shows it, e.g. after tasks
// changed
void memo_window_reload(const char *memo_name);

// Closes the window showing the given memo or comment
void memo_window_close(const char *memo_name);
