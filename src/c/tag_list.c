#include "tag_list.h"

#include "memo_list.h"
#include "remote_list.h"

#define TAG_LIST_MAX 30

// the items only take memory while the window is open
static RemoteList s_list = {
    .empty_text = "No tags",
};

static void select_row(uint16_t index, void *context) {
  remote_list_cancel(&s_list);
  memo_list_push_tag(s_list.items[index].id);
}

static void unload(void *context) {
  remote_list_cancel(&s_list);
  s_list.window = NULL;
  remote_list_free(&s_list);
}

void tag_list_push(void) {
  if (s_list.window || !remote_list_alloc(&s_list, TAG_LIST_MAX)) {
    return;
  }
  s_list.window = list_window_create(
      (ListWindowCallbacks){
          .get_count = remote_list_get_count,
          .get_row = remote_list_get_row,
          .select = select_row,
          .unload = unload,
      },
      &s_list);
  list_window_push(s_list.window);
  remote_list_load(&s_list, CommActionListTags, NULL, NULL);
}
