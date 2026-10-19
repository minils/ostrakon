#pragma once
#include <pebble.h>

// Opens the action menu for a memo: Tasks, Comments (if it has any),
// Append, Comment, Pin / Unpin, Archive, Delete
void memo_actions_open(const char *memo_name, bool has_comments);

// Opens the action menu for a comment of the memo memo_name: Tasks,
// Append, New comment (on the memo), Archive, Delete
void memo_actions_open_comment(const char *comment_name, const char *memo_name);
