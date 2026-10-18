#include "memo_window.h"

#include "comm.h"
#include "i18n.h"
#include "memo_actions.h"
#include "memo_view.h"
#include "ui.h"

static Window *s_window;
static Layer *s_title_bar;
static Layer *s_select_hint;
static Layer *s_loading_layer;
static MemoView *s_memo_view;
static char s_name[COMM_ID_SIZE];
static char s_content[COMM_TEXT_SIZE];
static bool s_loading;
static bool s_has_comments;

static void show_message(const char *message) {
  ui_copy_truncated(s_content, message, sizeof(s_content));
  memo_view_set_message(s_memo_view, s_content);
}

static void set_loading(bool loading) {
  s_loading = loading;
  ui_loading_layer_set_active(s_loading_layer, loading);
}

static void content_handler(DictionaryIterator *iter, void *context) {
  comm_finish();
  set_loading(false);
  if (!iter) {
    show_message(tr("No response from phone"));
  } else if (comm_response_failed(iter)) {
    show_message(comm_response_error(iter));
  } else {
    comm_copy_string(iter, MESSAGE_KEY_TEXT, s_content, sizeof(s_content));
    s_has_comments =
        comm_get_int(iter, MESSAGE_KEY_ITEM_FLAGS, 0) & MEMO_FLAG_HAS_COMMENTS;
    memo_view_set_lines(s_memo_view, s_content);
  }
}

static void select_click_handler(ClickRecognizerRef ref, void *context) {
  if (s_loading) {
    return;
  }
  memo_actions_open(s_name, false, s_has_comments);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void load_content(void) {
  show_message("");
  set_loading(comm_request(CommActionGet, s_name, NULL, content_handler, NULL));
  if (!s_loading) {
    show_message(tr("Phone not connected"));
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  GRect view_frame = bounds;
  view_frame.origin.y = UI_TITLE_BAR_HEIGHT;
  view_frame.size.h -= UI_TITLE_BAR_HEIGHT;
  s_memo_view =
      memo_view_create(root, view_frame, click_config_provider, window);

  s_title_bar = ui_title_bar_create(bounds, tr("Memo"));
  layer_add_child(root, s_title_bar);
  s_select_hint = ui_select_hint_create(bounds);
  layer_add_child(root, s_select_hint);
  s_loading_layer = ui_loading_layer_create(view_frame);
  layer_add_child(root, s_loading_layer);

  load_content();
}

static void window_unload(Window *window) {
  // only cancel our own request, not e.g. a pending archive
  if (s_loading) {
    comm_finish();
    s_loading = false;
  }
  ui_loading_layer_destroy(s_loading_layer);
  layer_destroy(s_select_hint);
  layer_destroy(s_title_bar);
  memo_view_destroy(s_memo_view);
  window_destroy(s_window);
  s_window = NULL;
}

void memo_window_push(const char *memo_name) {
  if (s_window) {
    return;
  }
  ui_copy_truncated(s_name, memo_name, sizeof(s_name));
  s_has_comments = false;

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
                                           .load = window_load,
                                           .unload = window_unload,
                                       });
  window_stack_push(s_window, true);
}

void memo_window_reload(const char *memo_name) {
  if (s_window && strcmp(s_name, memo_name) == 0) {
    load_content();
  }
}

void memo_window_close(const char *memo_name) {
  if (s_window && strcmp(s_name, memo_name) == 0) {
    window_stack_remove(s_window, true);
  }
}
