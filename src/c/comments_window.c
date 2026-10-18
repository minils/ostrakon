#include "comments_window.h"

#include "i18n.h"
#include "memo_actions.h"
#include "remote_list.h"
#include "ui.h"

// must match MAX_COMMENT_ITEMS in index.js
#define COMMENTS_MAX 20

// the items only take memory while the window is open
static RemoteList s_list = {
    .empty_text = "No comments yet",
};
static char s_memo_name[COMM_ID_SIZE];

static void select_row(uint16_t index, void *context) {
  if (remote_list_is_more_row(&s_list, index)) {
    return;
  }
  // an action replaces the pending request, so stop loading for good
  remote_list_cancel(&s_list);
  memo_actions_open_comment(s_list.items[index].id, s_memo_name, true);
}

static void unload(void *context) {
  remote_list_cancel(&s_list);
  s_list.window = NULL;
  free(s_list.items);
  s_list.items = NULL;
}

void comments_window_push(const char *memo_name) {
  if (s_list.window) {
    return;
  }
  s_list.items = malloc(COMMENTS_MAX * sizeof(RemoteListItem));
  if (!s_list.items) {
    return;
  }
  s_list.capacity = COMMENTS_MAX;
  ui_copy_truncated(s_memo_name, memo_name, sizeof(s_memo_name));
  s_list.window = list_window_create(
      (ListWindowCallbacks){
          .get_count = remote_list_get_count,
          .get_row = remote_list_get_row,
          .select = select_row,
          .unload = unload,
      },
      &s_list);
  list_window_set_title(s_list.window, tr("Comments"));
  list_window_push(s_list.window);
  remote_list_load(&s_list, CommActionListComments, s_memo_name, NULL);
}

void comments_window_reload(const char *memo_name) {
  if (s_list.window && strcmp(s_memo_name, memo_name) == 0) {
    remote_list_load(&s_list, CommActionListComments, s_memo_name, NULL);
  }
}

void comments_window_remove(const char *comment_name) {
  if (s_list.window) {
    remote_list_remove(&s_list, comment_name);
  }
}
