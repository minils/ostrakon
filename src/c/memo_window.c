#include "memo_window.h"

#include "comm.h"
#include "i18n.h"
#include "memo_actions.h"
#include "memo_view.h"
#include "ui.h"

// A memo or a comment, loaded from the phone. A comment can be opened on
// top of the memo it belongs to, so each has its own window.
typedef struct {
  Window *window;
  Layer *title_bar;
  Layer *select_hint;
  Layer *loading_layer;
  MemoView *memo_view;
  char name[COMM_ID_SIZE];
  char parent[COMM_ID_SIZE];  // the memo of a comment, empty for a memo
  char content[COMM_TEXT_SIZE];
  bool loading;
  bool has_comments;
} MemoScreen;

static MemoScreen s_memo;
static MemoScreen s_comment;

static void show_message(MemoScreen *screen, const char *message) {
  ui_copy_truncated(screen->content, message, sizeof(screen->content));
  memo_view_set_message(screen->memo_view, screen->content);
}

static void set_loading(MemoScreen *screen, bool loading) {
  screen->loading = loading;
  ui_loading_layer_set_active(screen->loading_layer, loading);
}

static void content_handler(DictionaryIterator *iter, void *context) {
  MemoScreen *screen = context;
  comm_finish();
  set_loading(screen, false);
  if (!iter) {
    show_message(screen, tr("No response from phone"));
  } else if (comm_response_failed(iter)) {
    show_message(screen, comm_response_error(iter));
  } else {
    comm_copy_string(iter, MESSAGE_KEY_TEXT, screen->content,
                     sizeof(screen->content));
    screen->has_comments =
        comm_get_int(iter, MESSAGE_KEY_ITEM_FLAGS, 0) & MEMO_FLAG_HAS_COMMENTS;
    memo_view_set_lines(screen->memo_view, screen->content);
  }
}

static void open_actions(MemoScreen *screen) {
  if (screen->loading) {
    return;
  }
  if (screen->parent[0]) {
    memo_actions_open_comment(screen->name, screen->parent, false);
  } else {
    memo_actions_open(screen->name, false, screen->has_comments);
  }
}

static void memo_select_click_handler(ClickRecognizerRef ref, void *context) {
  open_actions(&s_memo);
}

static void comment_select_click_handler(ClickRecognizerRef ref,
                                         void *context) {
  open_actions(&s_comment);
}

static void memo_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, memo_select_click_handler);
}

static void comment_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, comment_select_click_handler);
}

static void load_content(MemoScreen *screen) {
  show_message(screen, "");
  set_loading(screen, comm_request(CommActionGet, screen->name, NULL,
                                   content_handler, screen));
  if (!screen->loading) {
    show_message(screen, tr("Phone not connected"));
  }
}

static void window_load(Window *window) {
  MemoScreen *screen = window_get_user_data(window);
  bool is_comment = screen == &s_comment;
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  GRect view_frame = bounds;
  view_frame.origin.y = UI_TITLE_BAR_HEIGHT;
  view_frame.size.h -= UI_TITLE_BAR_HEIGHT;
  screen->memo_view = memo_view_create(
      root, view_frame,
      is_comment ? comment_click_config_provider : memo_click_config_provider,
      window);

  screen->title_bar =
      ui_title_bar_create(bounds, is_comment ? tr("Comment") : tr("Memo"));
  layer_add_child(root, screen->title_bar);
  screen->select_hint = ui_select_hint_create(bounds);
  layer_add_child(root, screen->select_hint);
  screen->loading_layer = ui_loading_layer_create(view_frame);
  layer_add_child(root, screen->loading_layer);

  load_content(screen);
}

static void window_unload(Window *window) {
  MemoScreen *screen = window_get_user_data(window);
  // only cancel our own request, not e.g. a pending archive
  if (screen->loading) {
    comm_finish();
    screen->loading = false;
  }
  ui_loading_layer_destroy(screen->loading_layer);
  layer_destroy(screen->select_hint);
  layer_destroy(screen->title_bar);
  memo_view_destroy(screen->memo_view);
  window_destroy(screen->window);
  screen->window = NULL;
}

static void push(MemoScreen *screen, const char *name, const char *parent) {
  if (screen->window) {
    return;
  }
  ui_copy_truncated(screen->name, name, sizeof(screen->name));
  ui_copy_truncated(screen->parent, parent ? parent : "",
                    sizeof(screen->parent));
  screen->has_comments = false;

  screen->window = window_create();
  window_set_user_data(screen->window, screen);
  window_set_window_handlers(screen->window, (WindowHandlers){
                                                 .load = window_load,
                                                 .unload = window_unload,
                                             });
  window_stack_push(screen->window, true);
}

static bool shows(const MemoScreen *screen, const char *name) {
  return screen->window && strcmp(screen->name, name) == 0;
}

void memo_window_push(const char *memo_name) { push(&s_memo, memo_name, NULL); }

void memo_window_push_comment(const char *comment_name, const char *memo_name) {
  push(&s_comment, comment_name, memo_name);
}

void memo_window_reload(const char *memo_name) {
  if (shows(&s_memo, memo_name)) {
    load_content(&s_memo);
  }
  if (shows(&s_comment, memo_name)) {
    load_content(&s_comment);
  }
}

void memo_window_close(const char *memo_name) {
  if (shows(&s_memo, memo_name)) {
    window_stack_remove(s_memo.window, true);
  }
  if (shows(&s_comment, memo_name)) {
    window_stack_remove(s_comment.window, true);
  }
}
