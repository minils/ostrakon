#include "memo_actions.h"

#include "actions.h"
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
  MemoActionOpen = 1,
  MemoActionTasks,
  MemoActionComments,
  MemoActionAppend,
  MemoActionComment,
  MemoActionPin,
  MemoActionArchive,
  MemoActionDelete,
};

static char s_name[COMM_ID_SIZE];
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
  memo_window_close(s_name);
  memo_list_remove(s_name);
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
    case MemoActionOpen:
      memo_window_push(s_name);
      break;
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
      compose_window_push_comment(s_name);
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

void memo_actions_open(const char *memo_name, bool include_open,
                       bool has_comments) {
  // labels are literals, so the items only need to live during this call
  ActionItem items[ACTIONS_MAX_ITEMS];
  int count = 0;
  if (include_open) {
    items[count++] = (ActionItem){.label = tr("Open"), .id = MemoActionOpen};
  }
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
  actions_open(items, count, action_chosen, NULL);
}
