#include "list_window.h"

#include "ui.h"

// The system default of 44px leaves the title and subtitle of a row
// cramped on larger displays; elsewhere the default is kept.
#if PBL_DISPLAY_WIDTH >= 200
#define DEFAULT_CELL_HEIGHT 56
#else
#define DEFAULT_CELL_HEIGHT 0
#endif

struct ListWindow {
  Window *window;
  MenuLayer *menu_layer;
  ListWindowCallbacks callbacks;
  void *context;
  char status[64];
  char title[64];
  Layer *title_bar;
  // while loading, an empty list shows animated dots instead of rows and
  // a filled list gets an extra row with the dots
  bool loading;
  Layer *loading_layer;
  AppTimer *loading_timer;
  int loading_frame;
};

// Menu callbacks

static uint16_t get_num_rows(MenuLayer *menu_layer, uint16_t section_index,
                             void *data) {
  ListWindow *list = data;
  uint16_t count = list->callbacks.get_count(list->context);
  if (count == 0) {
    return list->loading ? 0 : 1;  // one row for the status text
  }
  return count + (list->loading ? 1 : 0);
}

static void draw_row(GContext *ctx, const Layer *cell_layer,
                     MenuIndex *cell_index, void *data) {
  ListWindow *list = data;
  uint16_t count = list->callbacks.get_count(list->context);
  if (count == 0) {
    menu_cell_basic_draw(ctx, cell_layer, list->status, NULL, NULL);
    return;
  }
  if (cell_index->row >= count) {
    GRect bounds = layer_get_bounds(cell_layer);
    ui_draw_loading_dots(ctx, grect_center_point(&bounds), list->loading_frame,
                         menu_cell_layer_is_highlighted(cell_layer));
    return;
  }

  if (list->callbacks.draw_row) {
    list->callbacks.draw_row(ctx, cell_layer, cell_index->row, list->context);
    return;
  }

  const char *title = "";
  const char *subtitle = NULL;
  list->callbacks.get_row(cell_index->row, &title, &subtitle, list->context);
  menu_cell_basic_draw(ctx, cell_layer, title, subtitle, NULL);
}

static int16_t get_cell_height(MenuLayer *menu_layer, MenuIndex *cell_index,
                               void *data) {
  ListWindow *list = data;
  return list->callbacks.cell_height > 0 ? list->callbacks.cell_height
                                         : DEFAULT_CELL_HEIGHT;
}

static void select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index,
                              void *data) {
  ListWindow *list = data;
  if (list->callbacks.select_long &&
      cell_index->row < list->callbacks.get_count(list->context)) {
    list->callbacks.select_long(cell_index->row, list->context);
  }
}

static void select_click(MenuLayer *menu_layer, MenuIndex *cell_index,
                         void *data) {
  ListWindow *list = data;
  if (cell_index->row < list->callbacks.get_count(list->context)) {
    list->callbacks.select(cell_index->row, list->context);
  }
}

// Window

static void window_load(Window *window) {
  ListWindow *list = window_get_user_data(window);
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  // an optional title bar sits above the rows
  GRect menu_frame = bounds;
  if (list->title[0]) {
    menu_frame.origin.y = UI_TITLE_BAR_HEIGHT;
    menu_frame.size.h -= UI_TITLE_BAR_HEIGHT;
  }

  list->menu_layer = menu_layer_create(menu_frame);
  menu_layer_set_callbacks(
      list->menu_layer, list,
      (MenuLayerCallbacks){
          .get_num_rows = get_num_rows,
          .draw_row = draw_row,
          .get_cell_height =
              (list->callbacks.cell_height > 0 || DEFAULT_CELL_HEIGHT > 0)
                  ? get_cell_height
                  : NULL,
          .select_click = select_click,
          .select_long_click = select_long_click,
      });
  menu_layer_set_highlight_colors(list->menu_layer, UI_HIGHLIGHT_COLOR,
                                  UI_HIGHLIGHT_TEXT_COLOR);
  menu_layer_set_click_config_onto_window(list->menu_layer, window);
  layer_add_child(root, menu_layer_get_layer(list->menu_layer));

  list->loading_layer = ui_loading_layer_create(menu_frame);
  layer_add_child(root, list->loading_layer);

  if (list->title[0]) {
    list->title_bar = ui_title_bar_create(bounds, list->title);
    layer_add_child(root, list->title_bar);
  }
  ui_loading_layer_set_active(
      list->loading_layer,
      list->loading && list->callbacks.get_count(list->context) == 0);
}

static void window_appear(Window *window) {
  ListWindow *list = window_get_user_data(window);
  if (list->callbacks.appear) {
    list->callbacks.appear(list->context);
  }
  menu_layer_reload_data(list->menu_layer);
}

static void window_unload(Window *window) {
  ListWindow *list = window_get_user_data(window);
  if (list->loading_timer) {
    app_timer_cancel(list->loading_timer);
    list->loading_timer = NULL;
  }
  ui_loading_layer_destroy(list->loading_layer);
  menu_layer_destroy(list->menu_layer);
  if (list->title_bar) {
    layer_destroy(list->title_bar);
    list->title_bar = NULL;
  }
  // the unload callback may still end a request and update the loading
  // state, which must not touch the destroyed layers
  list->loading_layer = NULL;
  list->menu_layer = NULL;
  if (list->callbacks.unload) {
    list->callbacks.unload(list->context);
  }
  window_destroy(window);
  free(list);
}

ListWindow *list_window_create(ListWindowCallbacks callbacks, void *context) {
  ListWindow *list = calloc(1, sizeof(ListWindow));
  list->callbacks = callbacks;
  list->context = context;

  list->window = window_create();
  window_set_user_data(list->window, list);
  window_set_window_handlers(list->window, (WindowHandlers){
                                               .load = window_load,
                                               .appear = window_appear,
                                               .unload = window_unload,
                                           });
  return list;
}

void list_window_push(ListWindow *list) {
  window_stack_push(list->window, true);
}

void list_window_remove(ListWindow *list) {
  window_stack_remove(list->window, true);
}

void list_window_reload(ListWindow *list) {
  if (list->menu_layer) {
    menu_layer_reload_data(list->menu_layer);
  }
  if (list->loading_layer) {
    // the dots move into a row once the first items are there
    ui_loading_layer_set_active(
        list->loading_layer,
        list->loading && list->callbacks.get_count(list->context) == 0);
  }
}

// animates the dots in the loading row
static void loading_tick(void *context) {
  ListWindow *list = context;
  list->loading_frame++;
  if (list->menu_layer && list->callbacks.get_count(list->context) > 0) {
    menu_layer_reload_data(list->menu_layer);
  }
  list->loading_timer =
      app_timer_register(UI_LOADING_FRAME_MS, loading_tick, list);
}

void list_window_set_loading(ListWindow *list, bool loading) {
  list->loading = loading;
  if (loading && !list->loading_timer && list->menu_layer) {
    list->loading_timer =
        app_timer_register(UI_LOADING_FRAME_MS, loading_tick, list);
  } else if (!loading && list->loading_timer) {
    app_timer_cancel(list->loading_timer);
    list->loading_timer = NULL;
  }
  list_window_reload(list);
}

void list_window_set_title(ListWindow *list, const char *title) {
  ui_copy_truncated(list->title, title, sizeof(list->title));
}

void list_window_set_status(ListWindow *list, const char *status) {
  ui_copy_truncated(list->status, status, sizeof(list->status));
  list_window_reload(list);
}
