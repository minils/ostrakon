#pragma once
#include <pebble.h>

#define DRAFTS_MAX 3

void drafts_init(void);
int drafts_count(void);
const char *drafts_get(int index);

// Saves text as draft `index`, or as a new draft if index is -1.
// Returns the index of the saved draft, or -1 if no slot is free.
int drafts_save(int index, const char *text);

void drafts_delete(int index);
