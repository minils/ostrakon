#pragma once
#include <pebble.h>

// Checkboxes of one memo. Select toggles a task, synced to the server.
void tasks_window_push(const char *memo_name);
