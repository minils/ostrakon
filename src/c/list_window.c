#include "list_window.h"

#include "ui.h"

// The system default of 44px leaves the title and subtitle of a row
// cramped on larger displays; elsewhere the default is kept.
#if PBL_DISPLAY_WIDTH >= 200
#define DEFAULT_CELL_HEIGHT 56
#else
#define DEFAULT_CELL_HEIGHT 0
#endif

// An empty list shows its status, e.g. an error, over the whole list
// area like a Pebble dialog: a background color by its kind, the icon of
// the system dialogs for warnings and errors, and the wrapped text, all
// in the middle (the layout of simple_dialog in PebbleOS)
#if PBL_DISPLAY_WIDTH >= 200
#define STATUS_FONT_KEY FONT_KEY_GOTHIC_28
#define STATUS_ICON_SPACING PBL_IF_ROUND_ELSE(8, 16)
#else
#define STATUS_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define STATUS_ICON_SPACING PBL_IF_ROUND_ELSE(2, 4)
#endif
#define STATUS_INSET PBL_IF_ROUND_ELSE(16, 6)
// the system fonts leave room above the letters
#define STATUS_FONT_TOP 6
// black and white watches show dialogs black on white, so the black lines
// of the icons stay visible
#define STATUS_INFO_COLOR PBL_IF_COLOR_ELSE(UI_HIGHLIGHT_COLOR, GColorWhite)
#define STATUS_WARNING_COLOR PBL_IF_COLOR_ELSE(GColorChromeYellow, GColorWhite)
#define STATUS_ERROR_COLOR \
  PBL_IF_COLOR_ELSE(GColorDarkCandyAppleRed, GColorWhite)
#define STATUS_LIGHT_TEXT_COLOR PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack)

struct ListWindow {
  Window *window;
  MenuLayer *menu_layer;
  ListWindowCallbacks callbacks;
  void *context;
  char status[64];
  ListStatus status_kind;
  GDrawCommandImage *status_icon;  // of status_kind, NULL for none
  char title[64];
  Layer *title_bar;
  // while loading, an empty list shows animated dots instead of rows and
  // a filled list gets an extra row with the dots
  bool loading;
  Layer *loading_layer;
  Layer *status_layer;
  AppTimer *loading_timer;
  int loading_frame;
};

// Menu callbacks

static uint16_t get_num_rows(MenuLayer *menu_layer, uint16_t section_index,
                             void *data) {
  ListWindow *list = data;
  uint16_t count = list->callbacks.get_count(list->context);
  if (count == 0) {
    return 0;  // the status layer shows the status text
  }
  return count + (list->loading ? 1 : 0);
}

static void draw_row(GContext *ctx, const Layer *cell_layer,
                     MenuIndex *cell_index, void *data) {
  ListWindow *list = data;
  uint16_t count = list->callbacks.get_count(list->context);
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

// Status

static void status_update(Layer *layer, GContext *ctx) {
  ListWindow *list = *(ListWindow **)layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  GColor color = STATUS_INFO_COLOR;
  GColor text_color = STATUS_LIGHT_TEXT_COLOR;
  if (list->status_kind == ListStatusNoPhone ||
      list->status_kind == ListStatusOffline) {
    color = STATUS_WARNING_COLOR;
    text_color = GColorBlack;
  } else if (list->status_kind == ListStatusError) {
    color = STATUS_ERROR_COLOR;
  }
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  GFont font = fonts_get_system_font(STATUS_FONT_KEY);
  GRect text = grect_inset(bounds, GEdgeInsets(0, STATUS_INSET));
  GSize text_size = graphics_text_layout_get_content_size(
      list->status, font, text, GTextOverflowModeWordWrap,
      GTextAlignmentCenter);
  GSize icon_size = list->status_icon
                        ? gdraw_command_image_get_bounds_size(list->status_icon)
                        : GSizeZero;
  int16_t spacing = list->status_icon ? STATUS_ICON_SPACING : 0;

  // icon and text together in the middle
  int16_t height = icon_size.h + spacing + text_size.h;
  int16_t top = height < bounds.size.h ? (bounds.size.h - height) / 2 : 0;
  if (list->status_icon) {
    gdraw_command_image_draw(ctx, list->status_icon,
                             GPoint((bounds.size.w - icon_size.w) / 2, top));
  }
  text.origin.y = top + icon_size.h + spacing - STATUS_FONT_TOP;
  text.size.h = bounds.size.h - text.origin.y;
  graphics_context_set_text_color(ctx, text_color);
  graphics_draw_text(ctx, list->status, font, text, GTextOverflowModeWordWrap,
                     GTextAlignmentCenter, NULL);
}

static void update_status_layer(ListWindow *list) {
  if (list->status_layer) {
    layer_set_hidden(list->status_layer,
                     list->loading || !list->status[0] ||
                         list->callbacks.get_count(list->context) > 0);
    layer_mark_dirty(list->status_layer);
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

  list->status_layer = layer_create_with_data(menu_frame, sizeof(ListWindow *));
  *(ListWindow **)layer_get_data(list->status_layer) = list;
  layer_set_update_proc(list->status_layer, status_update);
  layer_add_child(root, list->status_layer);
  update_status_layer(list);

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
  layer_destroy(list->status_layer);
  if (list->status_icon) {
    gdraw_command_image_destroy(list->status_icon);
    list->status_icon = NULL;
  }
  menu_layer_destroy(list->menu_layer);
  if (list->title_bar) {
    layer_destroy(list->title_bar);
    list->title_bar = NULL;
  }
  // the unload callback may still end a request and update the loading
  // state, which must not touch the destroyed layers
  list->loading_layer = NULL;
  list->status_layer = NULL;
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
  update_status_layer(list);
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

void list_window_set_status(ListWindow *list, const char *status,
                            ListStatus kind) {
  ui_copy_truncated(list->status, status, sizeof(list->status));
  if (kind != list->status_kind || !list->status_icon) {
    if (list->status_icon) {
      gdraw_command_image_destroy(list->status_icon);
      list->status_icon = NULL;
    }
    // the icons PebbleOS shows for the same problems, e.g. in dictation
    if (kind == ListStatusNoPhone) {
      list->status_icon =
          gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_NO_PHONE);
    } else if (kind == ListStatusOffline) {
      list->status_icon = gdraw_command_image_create_with_resource(
          RESOURCE_ID_ICON_NO_INTERNET);
    } else if (kind == ListStatusError) {
      list->status_icon =
          gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_ERROR);
    }
  }
  list->status_kind = kind;
  list_window_reload(list);
}
