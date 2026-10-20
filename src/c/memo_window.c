#include "memo_window.h"

#include "cache.h"
#include "comm.h"
#include "i18n.h"
#include "memo_actions.h"
#include "memo_view.h"
#include "toast.h"
#include "ui.h"

// A memo or a comment, loaded from the phone. A comment can be opened on
// top of the memo it belongs to, so each has its own window.
//
// Long memos come in pages. Up to two pages are shown at a time; scrolling
// close to the end loads the next page and drops the first one, scrolling
// back to the start loads the previous page again.
typedef struct {
  Window *window;
  Layer *title_bar;
  Layer *select_hint;
  Layer *loading_layer;
  MemoView *memo_view;
  char name[COMM_ID_SIZE];
  char parent[COMM_ID_SIZE];  // the memo of a comment, empty for a memo
  // COMM_TEXT_SIZE bytes each; the second one only once it is needed
  char *pages[2];
  int page_blocks[2];  // blocks of each page in the memo view
  int num_pages;       // pages shown, 0 while a message is shown
  int first_page;      // number of the first page shown
  int loading_page;    // page being loaded, or -1
  bool more_below;
  bool has_comments;
} MemoScreen;

static MemoScreen s_memo;
static MemoScreen s_comment;

static void show_message(MemoScreen *screen, const char *message) {
  screen->num_pages = 0;
  ui_copy_truncated(screen->pages[0], message, COMM_TEXT_SIZE);
  memo_view_set_message(screen->memo_view, screen->pages[0]);
}

static void set_loading_page(MemoScreen *screen, int page) {
  screen->loading_page = page;
  // the first page is loaded in an empty view, further ones next to it
  ui_loading_layer_set_active(screen->loading_layer,
                              page >= 0 && screen->num_pages == 0);
  memo_view_set_loading(screen->memo_view, page >= 0 && screen->num_pages > 0);
}

static void stop_loading(MemoScreen *screen) {
  if (comm_is_pending(screen)) {
    comm_finish();
  }
  set_loading_page(screen, -1);
}

// Shows a loaded page; the first one replaces the message shown before
static void show_page(MemoScreen *screen, int page, const char *text,
                      bool more) {
  MemoView *view = screen->memo_view;
  if (screen->num_pages == 0) {
    ui_copy_truncated(screen->pages[0], text, COMM_TEXT_SIZE);
    screen->page_blocks[0] = memo_view_set_lines(view, screen->pages[0]);
    screen->num_pages = 1;
    screen->first_page = page;
    screen->more_below = more;
  } else if (page == screen->first_page + screen->num_pages) {
    if (screen->num_pages == 2) {
      // the first page makes room for the next one
      memo_view_remove_blocks(view, screen->page_blocks[0], false);
      char *buffer = screen->pages[0];
      screen->pages[0] = screen->pages[1];
      screen->pages[1] = buffer;
      screen->page_blocks[0] = screen->page_blocks[1];
      screen->first_page++;
    }
    ui_copy_truncated(screen->pages[1], text, COMM_TEXT_SIZE);
    screen->page_blocks[1] = memo_view_add_lines(view, screen->pages[1], true);
    screen->num_pages = 2;
    screen->more_below = more;
  } else if (page == screen->first_page - 1) {
    if (screen->num_pages == 2) {
      // the last page makes room for the previous one
      memo_view_remove_blocks(view, screen->page_blocks[1], true);
      screen->more_below = true;
    }
    char *buffer = screen->pages[1];
    screen->pages[1] = screen->pages[0];
    screen->pages[0] = buffer;
    screen->page_blocks[1] = screen->page_blocks[0];
    ui_copy_truncated(screen->pages[0], text, COMM_TEXT_SIZE);
    screen->page_blocks[0] = memo_view_add_lines(view, screen->pages[0], false);
    screen->num_pages = 2;
    screen->first_page = page;
  }
  memo_view_set_more(view, screen->first_page > 0, screen->more_below);
}

// Keeps what is shown if a further page cannot be loaded; scrolling there
// does not try again until a page at the other edge was loaded
static void page_failed(MemoScreen *screen, bool at_end, const char *error) {
  toast_show(error, false);
  if (at_end) {
    screen->more_below = false;
  }
  memo_view_set_more(screen->memo_view, at_end && screen->first_page > 0,
                     screen->more_below);
}

// Pages are kept in the cache on the watch for when the phone is away
static void cache_key(const MemoScreen *screen, int page, char *key,
                      size_t size) {
  snprintf(key, size, "g%d:%s", page, screen->name);
}

// Shows a page from the cache; returns false if it has none
static bool show_cached_page(MemoScreen *screen, int page) {
  char key[COMM_ID_SIZE + 8];
  cache_key(screen, page, key, sizeof(key));
  char *text = malloc(COMM_TEXT_SIZE);
  int32_t flags = 0;
  bool found = text && cache_load(key, &flags, text, COMM_TEXT_SIZE);
  if (found) {
    if (screen->num_pages == 0) {
      screen->has_comments = flags & MEMO_FLAG_HAS_COMMENTS;
      toast_show_status(tr("No phone:\nLoading last version"), UiStatusNoPhone);
    }
    show_page(screen, page, text, flags & MEMO_FLAG_MORE_TEXT);
  }
  free(text);
  return found;
}

static void content_handler(DictionaryIterator *iter, void *context) {
  MemoScreen *screen = context;
  int page = screen->loading_page;
  bool at_end = page > screen->first_page;
  comm_finish();
  set_loading_page(screen, -1);

  if (!iter && show_cached_page(screen, page)) {
    memo_view_check_edge(screen->memo_view, page == 0 || at_end);
    return;
  }
  if (!iter || comm_response_failed(iter)) {
    const char *error =
        iter ? comm_response_error(iter) : tr("No response from phone");
    if (screen->num_pages == 0) {
      show_message(screen, error);
    } else {
      page_failed(screen, at_end, error);
    }
    return;
  }

  Tuple *tuple = dict_find(iter, MESSAGE_KEY_TEXT);
  const char *text =
      tuple && tuple->type == TUPLE_CSTRING ? tuple->value->cstring : "";
  int32_t flags = comm_get_int(iter, MESSAGE_KEY_ITEM_FLAGS, 0);
  if (page == 0) {
    screen->has_comments = flags & MEMO_FLAG_HAS_COMMENTS;
  }
  char key[COMM_ID_SIZE + 8];
  cache_key(screen, page, key, sizeof(key));
  cache_store(key, flags, text, strlen(text));
  show_page(screen, page, text, flags & MEMO_FLAG_MORE_TEXT);
  // a short page may leave the view close to the edge still
  memo_view_check_edge(screen->memo_view, page == 0 || at_end);
}

// Requests a page, or shows it from the cache if the phone is not
// connected. Returns false if neither works.
static bool load_page(MemoScreen *screen, int page) {
  if (comm_request_page(CommActionGet, screen->name, NULL, page,
                        content_handler, screen)) {
    set_loading_page(screen, page);
    return true;
  }
  bool at_end = screen->num_pages == 0 || page > screen->first_page;
  if (!show_cached_page(screen, page)) {
    return false;
  }
  memo_view_check_edge(screen->memo_view, at_end);
  return true;
}

static void edge_reached(bool at_end, void *context) {
  MemoScreen *screen = context;
  if (screen->loading_page >= 0 || screen->num_pages == 0) {
    return;
  }
  if (!screen->pages[1]) {
    screen->pages[1] = malloc(COMM_TEXT_SIZE);
    if (!screen->pages[1]) {
      return;
    }
  }
  if (!load_page(screen, at_end ? screen->first_page + screen->num_pages
                                : screen->first_page - 1)) {
    page_failed(screen, at_end, tr("Phone not connected"));
  }
}

static void open_actions(MemoScreen *screen) {
  if (screen->num_pages == 0) {
    return;
  }
  // an action replaces the pending request; scrolling loads the page again
  stop_loading(screen);
  if (screen->parent[0]) {
    memo_actions_open_comment(screen->name, screen->parent);
  } else {
    memo_actions_open(screen->name, screen->has_comments);
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
  if (!load_page(screen, 0)) {
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
      window, edge_reached, screen);

  screen->title_bar =
      ui_title_bar_create(bounds, is_comment ? tr("Comment") : tr("Memo"));
  layer_add_child(root, screen->title_bar);
  screen->select_hint = ui_select_hint_create(bounds);
  layer_add_child(root, screen->select_hint);
  screen->loading_layer = ui_loading_layer_create(view_frame);
  layer_add_child(root, screen->loading_layer);

  load_content(screen);
}

static void window_appear(Window *window) {
  MemoScreen *screen = window_get_user_data(window);
  // a window on top may have replaced the request for a page; a further
  // page is loaded again when the view is scrolled
  if (screen->loading_page >= 0 && !comm_is_pending(screen)) {
    if (screen->num_pages == 0) {
      load_content(screen);
    } else {
      set_loading_page(screen, -1);
    }
  }
}

static void window_unload(Window *window) {
  MemoScreen *screen = window_get_user_data(window);
  // only cancel our own request, not e.g. a pending archive
  stop_loading(screen);
  ui_loading_layer_destroy(screen->loading_layer);
  layer_destroy(screen->select_hint);
  layer_destroy(screen->title_bar);
  memo_view_destroy(screen->memo_view);
  window_destroy(screen->window);
  screen->window = NULL;
  free(screen->pages[0]);
  free(screen->pages[1]);
  screen->pages[0] = screen->pages[1] = NULL;
}

static void push(MemoScreen *screen, const char *name, const char *parent) {
  if (screen->window) {
    return;
  }
  screen->pages[0] = malloc(COMM_TEXT_SIZE);
  if (!screen->pages[0]) {
    return;
  }
  screen->pages[1] = NULL;
  screen->num_pages = 0;
  screen->loading_page = -1;
  ui_copy_truncated(screen->name, name, sizeof(screen->name));
  ui_copy_truncated(screen->parent, parent ? parent : "",
                    sizeof(screen->parent));
  screen->has_comments = false;

  screen->window = window_create();
  window_set_user_data(screen->window, screen);
  window_set_window_handlers(screen->window, (WindowHandlers){
                                                 .load = window_load,
                                                 .appear = window_appear,
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
