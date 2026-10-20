#pragma once
#include <pebble.h>

// A small cache on the watch of what was last loaded from the phone, so
// lists and memos can still be shown while the phone is not connected.
// It only takes the persistent storage the drafts leave free; saving a
// draft drops the oldest entries if needed.

// The most bytes of data an entry keeps, enough for a page of a memo
#define CACHE_ENTRY_MAX 1536

// Stores data under key, replacing an older entry with the same key
void cache_store(const char *key, int32_t flags, const char *data, size_t size);

// Copies the entry for key into buffer, null-terminated and cut to fit.
// Returns false if there is none.
bool cache_load(const char *key, int32_t *flags, char *buffer, size_t size);

void cache_remove(const char *key);

// A version of the entry for key that changes with its data and flags, so
// the phone can answer that the watch already has what it would send; 0
// if there is no entry. Must match cacheVersion in index.js.
int32_t cache_version(const char *key);

// Drops the oldest entries until others bytes of other persisted data fit
// next to the cache
void cache_make_room(size_t others);
