#include "compose_window.h"

#include "actions.h"
#include "comm.h"
#include "comments_window.h"
#include "dictation.h"
#include "drafts.h"
#include "i18n.h"
#include "memo_window.h"
#include "text_view.h"
#include "toast.h"
#include "ui.h"

typedef enum {
  ComposeModeMemo,
  ComposeModeComment,
  ComposeModeAppend,
} ComposeMode;

enum {
  ComposeActionDictate = 1,
  ComposeActionSend,
  ComposeActionDiscard,
};

static Window *s_window;
static TextView s_text_view;

static ComposeMode s_mode;
static int s_draft_index;
static char s_target[COMM_ID_SIZE];
static char s_message[COMM_MESSAGE_SIZE];
static bool s_sending;

static void open_window(void);

// Display

static void update_display(void) {
  text_view_set_text(&s_text_view, s_message);
}

static void close_window(void) { window_stack_remove(s_window, true); }

// Dictation

static void dictation_done(const char *text) {
  if (!dictation_join(s_message, sizeof(s_message), text)) {
    toast_show(tr("Memo too long"), false);
    return;
  }
  // a new memo or comment only gets its window once there is text
  if (s_window) {
    update_display();
  } else {
    open_window();
  }
}

static Dictation s_dictation = {.handler = dictation_done, .confirm = true};

static void start_dictation(void) { dictation_start(&s_dictation); }

// Sending

// A new memo that could not reach the phone is kept as a draft, so it
// can be sent later from the drafts list
static void phone_unreachable(const char *message) {
  update_display();
  if (s_mode != ComposeModeMemo) {
    toast_show(message, false);
    return;
  }
  if (drafts_save(s_draft_index, s_message) < 0) {
    toast_show(tr("Drafts full.\nSend or discard."), false);
    return;
  }
  close_window();
  toast_show(tr("Phone unreachable.\nSaved as draft."), false);
}

static const char *success_message(void) {
  switch (s_mode) {
    case ComposeModeComment:
      return tr("Comment added");
    case ComposeModeAppend:
      return tr("Appended");
    default:
      return tr("Sent");
  }
}

static void send_handler(DictionaryIterator *iter, void *context) {
  comm_finish();
  s_sending = false;

  if (!iter) {
    phone_unreachable(tr("No response from phone"));
    return;
  }
  if (comm_response_failed(iter)) {
    update_display();
    toast_show(comm_response_error(iter), false);
    return;
  }

  if (s_draft_index >= 0) {
    drafts_delete(s_draft_index);
    s_draft_index = -1;
  }
  s_message[0] = '\0';
  close_window();
  // the open memo shows the new comment or appended text after a reload
  if (s_mode != ComposeModeMemo) {
    memo_window_reload(s_target);
    comments_window_reload(s_target);
  }
  // e.g. "Offline: memo queued" when the phone sends it later
  if (dict_find(iter, MESSAGE_KEY_TEXT)) {
    toast_show(comm_response_error(iter), true);
  } else {
    toast_show(success_message(), true);
  }
}

static void send(void) {
  if (ui_is_blank(s_message)) {
    toast_show(tr("Nothing to send"), false);
    return;
  }

  CommAction action = s_mode == ComposeModeComment  ? CommActionComment
                      : s_mode == ComposeModeAppend ? CommActionAppend
                                                    : CommActionCreate;
  const char *target = s_mode == ComposeModeMemo ? NULL : s_target;
  if (!comm_request(action, target, s_message, send_handler, NULL)) {
    phone_unreachable(tr("Phone not connected"));
    return;
  }

  s_sending = true;
  text_view_set_text(&s_text_view, tr("Sending..."));
}

// Input

static void action_chosen(int id, void *context) {
  switch (id) {
    case ComposeActionDictate:
      start_dictation();
      break;
    case ComposeActionSend:
      send();
      break;
    case ComposeActionDiscard:
      if (s_draft_index >= 0) {
        drafts_delete(s_draft_index);
        s_draft_index = -1;
      }
      s_message[0] = '\0';
      close_window();
      break;
  }
}

static void select_click_handler(ClickRecognizerRef ref, void *context) {
  if (s_sending) {
    return;
  }
  if (ui_is_blank(s_message)) {
    start_dictation();
    return;
  }

  const ActionItem items[] = {
      {.label = tr("Send"), .id = ComposeActionSend},
      {.label = tr("Dictate more"), .id = ComposeActionDictate},
      {.label = tr("Discard"),
       .id = ComposeActionDiscard,
       .confirm_label = tr("Yes, discard")},
  };
  actions_open(items, ARRAY_LENGTH(items), action_chosen, NULL);
}

static void back_click_handler(ClickRecognizerRef ref, void *context) {
  // leaving while sending could store a draft of a memo that gets sent
  if (s_sending) {
    return;
  }

  if (ui_is_blank(s_message)) {
    close_window();
    return;
  }

  if (s_mode != ComposeModeMemo) {
    close_window();
    toast_show(s_mode == ComposeModeComment ? tr("Comment discarded")
                                            : tr("Append discarded"),
               false);
    return;
  }

  if (drafts_save(s_draft_index, s_message) < 0) {
    toast_show(tr("Drafts full.\nSend or discard."), false);
    return;
  }
  close_window();
  toast_show(tr("Saved as draft"), true);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

// Window

static void window_load(Window *window) {
  text_view_create(&s_text_view, window,
                   s_mode == ComposeModeComment  ? tr("Comment")
                   : s_mode == ComposeModeAppend ? tr("Append")
                                                 : tr("Draft"),
                   UI_TEXT_FONT_KEY, GTextAlignmentCenter,
                   click_config_provider);
  update_display();
}

static void window_unload(Window *window) {
  text_view_destroy(&s_text_view);
  dictation_release(&s_dictation);
  if (s_sending) {
    comm_finish();
    s_sending = false;
  }
  window_destroy(s_window);
  s_window = NULL;
}

static void open_window(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = window_load,
                                           .unload = window_unload,
                                       });
  window_stack_push(s_window, true);
}

static void delayed_dictation(void *data) { start_dictation(); }

static void push(ComposeMode mode, int draft_index, const char *text,
                 const char *target, bool dictate) {
  if (s_window) {
    return;
  }
  s_mode = mode;
  s_draft_index = draft_index;
  s_sending = false;
  ui_copy_truncated(s_message, text ? text : "", sizeof(s_message));
  ui_copy_truncated(s_target, target ? target : "", sizeof(s_target));

  if (dictate) {
    // the window opens after a successful dictation; cancelling it
    // returns straight to where the user came from. The short delay lets
    // a pending window transition (e.g. at quick launch) finish first.
    app_timer_register(200, delayed_dictation, NULL);
  } else {
    open_window();
  }
}

void compose_window_push_new(void) {
  push(ComposeModeMemo, -1, NULL, NULL, true);
}

void compose_window_push_draft(int draft_index) {
  push(ComposeModeMemo, draft_index, drafts_get(draft_index), NULL, false);
}

void compose_window_push_comment(const char *memo_name) {
  push(ComposeModeComment, -1, NULL, memo_name, true);
}

void compose_window_push_append(const char *memo_name) {
  push(ComposeModeAppend, -1, NULL, memo_name, true);
}
