#pragma once
#include <pebble.h>

// New memo: starts dictation and opens the window once there is text.
// Unsent text is kept as a draft when leaving.
void compose_window_push_new(void);

// Continue editing a saved draft
void compose_window_push_draft(int draft_index);

// Comment on the memo with the given name, also starting with dictation
void compose_window_push_comment(const char *memo_name);

// Add dictated text to the end of the memo with the given name
void compose_window_push_append(const char *memo_name);
