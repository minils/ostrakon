#include "toast.h"

#include "ui.h"

#define TOAST_DURATION_MS 1500

static Window *s_window;
static TextLayer *s_text_layer;
static AppTimer *s_timer;
static char s_text[64];

static void timer_callback(void *data) {
  s_timer = NULL;
  if (s_window) {
    window_stack_remove(s_window, true);
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);
  int16_t inset = PBL_IF_ROUND_ELSE(bounds.size.w / 8, 4);

  s_text_layer = text_layer_create(
      GRect(inset, bounds.size.h / 2 - 34, bounds.size.w - 2 * inset, 68));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  text_layer_set_font(s_text_layer, fonts_get_system_font(UI_TEXT_FONT_KEY));
  text_layer_set_text(s_text_layer, s_text);
  layer_add_child(root, text_layer_get_layer(s_text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  window_destroy(s_window);
  s_window = NULL;
  if (s_timer) {
    app_timer_cancel(s_timer);
    s_timer = NULL;
  }
}

static void show(const char *text) {
  ui_copy_truncated(s_text, text, sizeof(s_text));

  if (s_window) {
    text_layer_set_text(s_text_layer, s_text);
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
  show(text);
  ui_vibrate(success);
}

void toast_show_quiet(const char *text) { show(text); }
