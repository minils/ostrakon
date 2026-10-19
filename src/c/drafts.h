#pragma once
#include <pebble.h>

#define DRAFTS_MAX 3

void drafts_init(void);
int drafts_count(void);

// Copies draft `index` into buffer, cut to fit at a character boundary;
// an index without a draft, e.g. -1, gives an empty text
void drafts_read(int index, char *buffer, size_t size);

// Saves text as draft `index`, or as a new draft if index is -1.
// Returns the index of the saved draft, or -1 if no slot is free.
int drafts_save(int index, const char *text);

void drafts_delete(int index);

// Bytes the stored drafts take in persistent storage
size_t drafts_stored_size(void);
