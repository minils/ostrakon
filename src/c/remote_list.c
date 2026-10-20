#include "remote_list.h"

#include "cache.h"
#include "i18n.h"
#include "toast.h"
#include "ui.h"

static void set_loading(RemoteList *list, bool loading) {
  list->loading = loading;
  list_window_set_loading(list->window, loading);
}

static void finish(RemoteList *list) {
  comm_finish();
  set_loading(list, false);
}

// Errors replace the empty list; with items shown they become a toast
static void show_error(RemoteList *list, const char *error, UiStatus kind) {
  if (list->count == 0) {
    list_window_set_status(list->window, error, kind);
  } else {
    toast_show(error, false);
    list_window_reload(list->window);
  }
}

/* ---------- Packed items ---------- */
// The phone packs several items into one message to save round trips:
// records separated by RS, fields (id, title, subtitle, flags) by US.
#define RECORD_SEPARATOR '\x1e'
#define FIELD_SEPARATOR '\x1f'

// Copies the field starting at p into dst and returns where the next one
// starts; *end is set to the separator that ended the field
static const char *read_field(const char *p, char *dst, size_t size,
                              char *end) {
  const char *start = p;
  while (*p && *p != FIELD_SEPARATOR && *p != RECORD_SEPARATOR) {
    p++;
  }
  size_t len = p - start;
  if (len >= size) {
    len = size - 1;
    // step back over UTF-8 continuation bytes so no character is split
    while (len > 0 && ((uint8_t)start[len] & 0xC0) == 0x80) {
      len--;
    }
  }
  memcpy(dst, start, len);
  dst[len] = '\0';
  *end = *p;
  return *p ? p + 1 : p;
}

// Reads the records of one message into the items starting at index;
// returns the index after the last item read
static int32_t unpack_items(RemoteList *list, const char *p, int32_t index) {
  char flags[12];
  while (*p && index < list->capacity) {
    RemoteListItem *item = &list->items[index];
    char end = FIELD_SEPARATOR;
    item->id[0] = item->title[0] = item->subtitle[0] = flags[0] = '\0';

    p = read_field(p, item->id, sizeof(item->id), &end);
    if (end == FIELD_SEPARATOR) {
      p = read_field(p, item->title, sizeof(item->title), &end);
    }
    if (end == FIELD_SEPARATOR) {
      p = read_field(p, item->subtitle, sizeof(item->subtitle), &end);
    }
    if (end == FIELD_SEPARATOR) {
      p = read_field(p, flags, sizeof(flags), &end);
    }
    item->flags = 0;
    for (const char *c = flags; *c >= '0' && *c <= '9'; c++) {
      item->flags = item->flags * 10 + (*c - '0');
    }
    index++;
  }
  return index;
}

/* ---------- Cache on the watch ---------- */
// The first page of a list is kept for when the phone is not connected.
// Search results are not worth the room.

static bool cache_key(const RemoteList *list, char *key, size_t size) {
  if (list->action == CommActionSearch) {
    return false;
  }
  snprintf(key, size, "l%d:%s", (int)list->action, list->target);
  return true;
}

// Packs the items that fit into an entry, the same way the phone does
static void store_items(RemoteList *list) {
  char key[COMM_ID_SIZE + 8];
  char *data = malloc(CACHE_ENTRY_MAX);
  if (!data || !cache_key(list, key, sizeof(key))) {
    free(data);
    return;
  }
  size_t size = 0;
  for (uint16_t i = 0; i < list->count; i++) {
    const RemoteListItem *item = &list->items[i];
    char flags[12];
    snprintf(flags, sizeof(flags), "%d", (int)item->flags);
    size_t len = strlen(item->id) + strlen(item->title) +
                 strlen(item->subtitle) + strlen(flags) + 4;
    if (size + len > CACHE_ENTRY_MAX) {
      break;
    }
    size += snprintf(data + size, CACHE_ENTRY_MAX - size,
                     "%s%s\x1f%s\x1f%s\x1f%s", i > 0 ? "\x1e" : "", item->id,
                     item->title, item->subtitle, flags);
  }
  cache_store(key, 0, data, size);
  free(data);
}

// Shows the first page from the cache; returns false if it has none
static bool show_cached_items(RemoteList *list) {
  char key[COMM_ID_SIZE + 8];
  char *data = malloc(CACHE_ENTRY_MAX + 1);
  int32_t flags;
  bool found = data && cache_key(list, key, sizeof(key)) &&
               cache_load(key, &flags, data, CACHE_ENTRY_MAX + 1);
  if (found) {
    list->count = unpack_items(list, data, 0);
    list->expected = list->count;
    list->has_more = false;
    if (list->count == 0) {
      list_window_set_status(list->window, tr(list->empty_text), UiStatusInfo);
    }
    list_window_reload(list->window);
    toast_show_status(tr("No phone: loading last version"), UiStatusNoPhone);
  }
  free(data);
  return found;
}

static void response_handler(DictionaryIterator *iter, void *context) {
  RemoteList *list = context;

  if (!iter) {
    set_loading(list, false);
    if (list->page_start > 0 || !show_cached_items(list)) {
      show_error(list, tr("No response from phone"), UiStatusNoPhone);
    }
    return;
  }
  if (comm_response_failed(iter)) {
    finish(list);
    show_error(
        list, comm_response_error(iter),
        comm_response_is_warning(iter) ? UiStatusOffline : UiStatusError);
    return;
  }

  // the first message of a page announces its number of items
  int32_t count = comm_get_int(iter, MESSAGE_KEY_LIST_COUNT, -1);
  if (count >= 0) {
    // the phone adds a note if the list comes from its offline cache
    if (dict_find(iter, MESSAGE_KEY_TEXT)) {
      toast_show_status(comm_response_error(iter), UiStatusOffline);
    }
    int32_t end = list->page_start + count;
    list->expected = end < list->capacity ? end : list->capacity;
    list->has_more = comm_get_int(iter, MESSAGE_KEY_ITEM_FLAGS, 0) != 0 &&
                     end < list->capacity;
    if (list->expected <= list->page_start) {
      finish(list);
      if (list->page_start == 0) {
        store_items(list);
      }
      if (list->count == 0) {
        list_window_set_status(list->window, tr(list->empty_text),
                               UiStatusInfo);
      }
      list_window_reload(list->window);
      return;
    }
  }

  // the first message can already carry items
  Tuple *items = dict_find(iter, MESSAGE_KEY_ITEMS);
  if (!items || items->type != TUPLE_CSTRING) {
    return;
  }
  int32_t index =
      list->page_start + comm_get_int(iter, MESSAGE_KEY_ITEM_INDEX, 0);
  if (index < list->page_start || index >= list->capacity) {
    return;
  }

  // items arrive in order, so they can be shown while the rest loads
  list->count = unpack_items(list, items->value->cstring, index);
  if (list->count >= list->expected) {
    finish(list);
    if (list->page_start == 0) {
      store_items(list);
    }
  }
  list_window_reload(list->window);
}

static void request_page(RemoteList *list) {
  list->page_start = list->count;
  list->expected = list->count;
  set_loading(list, comm_request_page(
                        list->action, list->target[0] ? list->target : NULL,
                        list->text[0] ? list->text : NULL, list->page_start,
                        response_handler, list));
  if (!list->loading && (list->page_start > 0 || !show_cached_items(list))) {
    show_error(list, tr("Phone not connected"), UiStatusNoPhone);
  }
}

bool remote_list_alloc(RemoteList *list, uint16_t capacity) {
  list->items = malloc(capacity * sizeof(RemoteListItem));
  list->capacity = list->items ? capacity : 0;
  list->count = 0;
  return list->items != NULL;
}

void remote_list_free(RemoteList *list) {
  free(list->items);
  list->items = NULL;
  list->capacity = 0;
  list->count = 0;
}

void remote_list_load(RemoteList *list, CommAction action, const char *target,
                      const char *text) {
  list->count = 0;
  list->has_more = false;
  list->action = action;
  ui_copy_truncated(list->target, target ? target : "", sizeof(list->target));
  ui_copy_truncated(list->text, text ? text : "", sizeof(list->text));
  request_page(list);
}

void remote_list_load_more(RemoteList *list) {
  if (list->loading || !list->has_more) {
    return;
  }
  request_page(list);
  list_window_reload(list->window);
}

void remote_list_cancel(RemoteList *list) {
  if (list->loading) {
    finish(list);
  }
}

void remote_list_remove(RemoteList *list, const char *id) {
  for (uint16_t i = 0; i < list->count; i++) {
    if (strcmp(list->items[i].id, id) != 0) {
      continue;
    }
    memmove(&list->items[i], &list->items[i + 1],
            (list->count - i - 1) * sizeof(RemoteListItem));
    list->count--;
    list->expected--;
    if (list->count == 0) {
      list_window_set_status(list->window, tr(list->empty_text), UiStatusInfo);
    }
    list_window_reload(list->window);
    return;
  }
}

// while a page loads, the list window shows its loading row instead
static bool show_more_row(const RemoteList *list) {
  return list->count > 0 && list->has_more && !list->loading;
}

bool remote_list_is_more_row(const RemoteList *list, uint16_t index) {
  return index == list->count && show_more_row(list);
}

uint16_t remote_list_get_count(void *context) {
  RemoteList *list = context;
  return list->count + (show_more_row(list) ? 1 : 0);
}

void remote_list_get_row(uint16_t index, const char **title,
                         const char **subtitle, void *context) {
  RemoteList *list = context;
  if (remote_list_is_more_row(list, index)) {
    *title = tr("Load more");
    *subtitle = NULL;
    return;
  }
  *title = list->items[index].title;
  *subtitle = list->items[index].subtitle;
}
