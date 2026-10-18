#pragma once
#include <pebble.h>

// Lists of memos, loaded page by page from the phone
void memo_list_push_recent(void);
void memo_list_push_tag(const char *tag);
void memo_list_push_search(const char *query);
void memo_list_push_pinned(void);

// Removes a memo from the open list, e.g. after it was archived
void memo_list_remove(const char *memo_name);

// Removes the memo from the open list if it is the list of pinned memos
void memo_list_unpinned(const char *memo_name);
