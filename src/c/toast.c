#include "toast.h"

#include "ui.h"

#define TOAST_DURATION_MS 1500

static Window *s_window;
static TextLayer *s_text_layer;
static Layer *s_status_layer;
static AppTimer *s_timer;
static char s_text[64];
// a status toast is drawn as a status screen instead of plain text
static bool s_is_status;
static UiStatus s_kind;
static GDrawCommandImage *s_icon;

static void timer_callback(void *data) {
  s_timer = NULL;
  if (s_window) {
    window_stack_remove(s_window, true);
  }
}

static void status_update(Layer *layer, GContext *ctx) {
  ui_draw_status(ctx, layer_get_bounds(layer), s_kind, s_icon, s_text);
}

static void update_layers(void) {
  text_layer_set_text(s_text_layer, s_text);
  layer_set_hidden(text_layer_get_layer(s_text_layer), s_is_status);
  layer_set_hidden(s_status_layer, !s_is_status);
  layer_mark_dirty(s_status_layer);
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  int16_t inset = PBL_IF_ROUND_ELSE(bounds.size.w / 8, 4);

  s_text_layer = text_layer_create(
      GRect(inset, bounds.size.h / 2 - 34, bounds.size.w - 2 * inset, 68));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_text_layer, fonts_get_system_font(UI_TEXT_FONT_KEY));
  layer_add_child(root, text_layer_get_layer(s_text_layer));

  s_status_layer = layer_create(bounds);
  layer_set_update_proc(s_status_layer, status_update);
  layer_add_child(root, s_status_layer);
  update_layers();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  layer_destroy(s_status_layer);
  if (s_icon) {
    gdraw_command_image_destroy(s_icon);
    s_icon = NULL;
  }
  window_destroy(s_window);
  s_window = NULL;
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
}

static void show(const char *text, bool is_status, UiStatus kind) {
  ui_copy_truncated(s_text, text, sizeof(s_text));
  if (s_icon && (!is_status || kind != s_kind)) {
    gdraw_command_image_destroy(s_icon);
    s_icon = NULL;
  }
  if (is_status && !s_icon) {
    s_icon = ui_status_icon_create(kind);
  }
  s_is_status = is_status;
  s_kind = kind;

  if (s_window) {
    update_layers();
  } else {
    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers){
                                             .load = window_load,
                                             .unload = window_unload,
                                         });
    window_stack_push(s_window, true);
  }

  if (s_timer) {
    app_timer_reschedule(s_timer, TOAST_DURATION_MS);
  } else {
    s_timer = app_timer_register(TOAST_DURATION_MS, timer_callback, NULL);
  }
}

void toast_show(const char *text, bool success) {
  show(text, false, UiStatusInfo);
  ui_vibrate(success);
}

void toast_show_status(const char *text, UiStatus kind) {
  show(text, true, kind);
}
