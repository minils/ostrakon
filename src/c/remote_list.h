#pragma once
#include <pebble.h>

#include "comm.h"
#include "list_window.h"

typedef struct {
  char id[COMM_ID_SIZE];
  char title[COMM_TITLE_SIZE];
  char subtitle[COMM_SUBTITLE_SIZE];
  int32_t flags;  // meaning depends on the list, e.g. task done
} RemoteListItem;

// Items of a list loaded from the phone, shown in a ListWindow. Lists the
// phone pages show a "Load more" row after the last item.
typedef struct {
  RemoteListItem *items;  // see remote_list_alloc
  uint16_t capacity;
  uint16_t count;
  uint16_t expected;
  uint16_t page_start;  // index of the first item of the page being loaded
  bool loading;
  bool has_more;
  int32_t version;  // of the cached first page sent with its request, or 0
  const char *empty_text;
  ListWindow *window;
  // the request, repeated for further pages
  CommAction action;
  char target[COMM_ID_SIZE];
  char text[COMM_TITLE_SIZE];
} RemoteList;

// Allocates room for capacity items while the list is shown. Returns
// false if there is not enough memory.
bool remote_list_alloc(RemoteList *list, uint16_t capacity);

// Frees the items, e.g. when the window is closed
void remote_list_free(RemoteList *list);

// Clears the list and requests its first page from the phone. target and
// text may be NULL.
void remote_list_load(RemoteList *list, CommAction action, const char *target,
                      const char *text);

// Requests the next page, appending to the items
void remote_list_load_more(RemoteList *list);

// Stops loading, e.g. when the window is closed
void remote_list_cancel(RemoteList *list);

// Removes the item with the given id, e.g. after it was archived
void remote_list_remove(RemoteList *list, const char *id);

// True for the "Load more" row after the items
bool remote_list_is_more_row(const RemoteList *list, uint16_t index);

// ListWindowCallbacks helpers; the context must be the RemoteList
uint16_t remote_list_get_count(void *context);
void remote_list_get_row(uint16_t index, const char **title,
                         const char **subtitle, void *context);
