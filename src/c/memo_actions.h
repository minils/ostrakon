#pragma once
#include <pebble.h>

// Opens the action menu for a memo: Open (optional), Tasks, Comments (if
// it has any), Append, Comment, Pin / Unpin, Archive, Delete
void memo_actions_open(const char *memo_name, bool include_open,
                       bool has_comments);
