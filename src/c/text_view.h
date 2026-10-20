#pragma once
#include <pebble.h>

// A scrollable text, used by the compose and memo windows
typedef struct {
  Layer *title_bar;
  ScrollLayer *scroll_layer;
  TextLayer *text_layer;
  Layer *scroll_arrow;
  Layer *select_hint;
} TextView;

// Shows a title bar above the text and a select hint at the right edge.
// click_config_provider adds the window's own buttons; up and down scroll.
// The title is not copied.
void text_view_create(TextView *view, Window *window, const char *title,
                      const char *font_key, GTextAlignment alignment,
                      ClickConfigProvider click_config_provider);
void text_view_destroy(TextView *view);

// The text is not copied and must stay valid while it is shown
void text_view_set_text(TextView *view, const char *text);
