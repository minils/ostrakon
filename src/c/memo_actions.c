#include "memo_actions.h"

#include "actions.h"
#include "cache.h"
#include "comm.h"
#include "comments_window.h"
#include "compose_window.h"
#include "i18n.h"
#include "memo_list.h"
#include "memo_window.h"
#include "tasks_window.h"
#include "toast.h"
#include "ui.h"

enum {
  MemoActionTasks = 1,
  MemoActionComments,
  MemoActionAppend,
  MemoActionComment,
  MemoActionPin,
  MemoActionArchive,
  MemoActionDelete,
};

static char s_name[COMM_ID_SIZE];
static char s_parent[COMM_ID_SIZE];  // the memo of a comment, else empty
static int s_pending_action;

static void modify_handler(DictionaryIterator *iter, void *context) {
  comm_finish();

  if (!iter) {
    toast_show(tr("No response from phone"), false);
    return;
  }
  if (comm_response_failed(iter)) {
    toast_show(comm_response_error(iter), false);
    return;
  }

  // the memo is gone from the normal list either way
  char key[COMM_ID_SIZE + 8];
  snprintf(key, sizeof(key), "g0:%s", s_name);  // see memo_window.c
  cache_remove(key);
  memo_window_close(s_name);
  memo_list_remove(s_name);
  comments_window_remove(s_name);
  toast_show(
      s_pending_action == MemoActionArchive ? tr("Archived") : tr("Deleted"),
      true);
}

static void pin_handler(DictionaryIterator *iter, void *context) {
  comm_finish();

  if (!iter) {
    toast_show(tr("No response from phone"), false);
    return;
  }
  if (comm_response_failed(iter)) {
    toast_show(comm_response_error(iter), false);
    return;
  }
  if (comm_get_int(iter, MESSAGE_KEY_ITEM_FLAGS, 1) == 0) {
    memo_list_unpinned(s_name);
  }
  toast_show(comm_response_error(iter), true);  // tr("Pinned") or "Unpinned"
}

static void modify(CommAction action) {
  if (!comm_request(action, s_name, NULL, modify_handler, NULL)) {
    toast_show(tr("Phone not connected"), false);
  }
}

static void action_chosen(int id, void *context) {
  switch (id) {
    case MemoActionTasks:
      tasks_window_push(s_name);
      break;
    case MemoActionComments:
      comments_window_push(s_name);
      break;
    case MemoActionArchive:
      s_pending_action = id;
      modify(CommActionArchive);
      break;
    case MemoActionDelete:
      s_pending_action = id;
      modify(CommActionDelete);
      break;
    case MemoActionComment:
      // a comment on a comment goes to the memo
      compose_window_push_comment(s_parent[0] ? s_parent : s_name);
      break;
    case MemoActionAppend:
      compose_window_push_append(s_name);
      break;
    case MemoActionPin:
      if (!comm_request(CommActionTogglePin, s_name, NULL, pin_handler, NULL)) {
        toast_show(tr("Phone not connected"), false);
      }
      break;
  }
}

void memo_actions_open(const char *memo_name, bool has_comments) {
  // labels are literals, so the items only need to live during this call
  ActionItem items[ACTIONS_MAX_ITEMS];
  int count = 0;
  items[count++] = (ActionItem){.label = tr("Tasks"), .id = MemoActionTasks};
  if (has_comments) {
    items[count++] =
        (ActionItem){.label = tr("Comments"), .id = MemoActionComments};
  }
  items[count++] = (ActionItem){.label = tr("Append"), .id = MemoActionAppend};
  items[count++] =
      (ActionItem){.label = tr("Comment"), .id = MemoActionComment};
  items[count++] =
      (ActionItem){.label = tr("Pin / Unpin"), .id = MemoActionPin};
  items[count++] =
      (ActionItem){.label = tr("Archive"), .id = MemoActionArchive};
  items[count++] = (ActionItem){.label = tr("Delete"),
                                .id = MemoActionDelete,
                                .confirm_label = tr("Yes, delete")};

  ui_copy_truncated(s_name, memo_name, sizeof(s_name));
  s_parent[0] = '\0';
  actions_open(items, count, action_chosen, NULL);
}

void memo_actions_open_comment(const char *comment_name,
                               const char *memo_name) {
  ActionItem items[ACTIONS_MAX_ITEMS];
  int count = 0;
  items[count++] = (ActionItem){.label = tr("Tasks"), .id = MemoActionTasks};
  items[count++] = (ActionItem){.label = tr("Append"), .id = MemoActionAppend};
  items[count++] =
      (ActionItem){.label = tr("New comment"), .id = MemoActionComment};
  items[count++] =
      (ActionItem){.label = tr("Archive"), .id = MemoActionArchive};
  items[count++] = (ActionItem){.label = tr("Delete"),
                                .id = MemoActionDelete,
                                .confirm_label = tr("Yes, delete")};

  ui_copy_truncated(s_name, comment_name, sizeof(s_name));
  ui_copy_truncated(s_parent, memo_name, sizeof(s_parent));
  actions_open(items, count, action_chosen, NULL);
}
