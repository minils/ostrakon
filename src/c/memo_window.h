#pragma once
#include <pebble.h>

// Shows the full text of a memo, loaded from the phone
void memo_window_push(const char *memo_name);

// Loads the memo again if the window shows it, e.g. after tasks changed
void memo_window_reload(const char *memo_name);

// Closes the memo window if it shows the given memo
void memo_window_close(const char *memo_name);
