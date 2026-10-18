#include "ui.h"

#include "settings.h"

// Title bar

typedef struct {
  const char *title;
} TitleBarData;

static void title_bar_update(Layer *layer, GContext *ctx) {
  TitleBarData *data = layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);

  graphics_context_set_fill_color(ctx, UI_HIGHLIGHT_COLOR);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  GFont font =
      fonts_get_system_font(PBL_DISPLAY_WIDTH >= 200 ? FONT_KEY_GOTHIC_18_BOLD
                                                     : FONT_KEY_GOTHIC_14_BOLD);
  graphics_context_set_text_color(ctx, UI_HIGHLIGHT_TEXT_COLOR);
  // the system fonts have some space above the letters
  graphics_draw_text(
      ctx, data->title, font, GRect(0, -3, bounds.size.w, bounds.size.h + 3),
      GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
}

Layer *ui_title_bar_create(GRect window_bounds, const char *title) {
  Layer *layer = layer_create_with_data(
      GRect(0, 0, window_bounds.size.w, UI_TITLE_BAR_HEIGHT),
      sizeof(TitleBarData));
  ((TitleBarData *)layer_get_data(layer))->title = title;
  layer_set_update_proc(layer, title_bar_update);
  return layer;
}

// Select hint

static void select_hint_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  // only the left half of the circle is inside the layer
  graphics_fill_circle(ctx, GPoint(bounds.size.w, bounds.size.h / 2),
                       UI_SELECT_HINT_RADIUS);
}

Layer *ui_select_hint_create(GRect window_bounds) {
  Layer *layer =
      layer_create(GRect(window_bounds.size.w - UI_SELECT_HINT_RADIUS, 0,
                         UI_SELECT_HINT_RADIUS, window_bounds.size.h));
  layer_set_update_proc(layer, select_hint_update);
  return layer;
}

// Loading animation

#if PBL_DISPLAY_WIDTH >= 200
#define LOADING_DOT_RADIUS 5
#define LOADING_DOT_SPACING 18
#else
#define LOADING_DOT_RADIUS 4
#define LOADING_DOT_SPACING 14
#endif

void ui_draw_loading_dots(GContext *ctx, GPoint center, int frame,
                          bool on_highlight) {
  for (int i = 0; i < 3; i++) {
    GPoint dot = GPoint(center.x + (i - 1) * LOADING_DOT_SPACING, center.y);
    bool active = i == frame % 3;
#ifdef PBL_COLOR
    GColor active_color = on_highlight ? GColorWhite : UI_HIGHLIGHT_COLOR;
    GColor idle_color = on_highlight ? GColorLightGray : GColorLightGray;
    graphics_context_set_fill_color(ctx, active ? active_color : idle_color);
    graphics_fill_circle(ctx, dot, LOADING_DOT_RADIUS);
#else
    GColor color = on_highlight ? GColorWhite : GColorBlack;
    graphics_context_set_fill_color(ctx, color);
    graphics_context_set_stroke_color(ctx, color);
    if (active) {
      graphics_fill_circle(ctx, dot, LOADING_DOT_RADIUS);
    } else {
      graphics_draw_circle(ctx, dot, LOADING_DOT_RADIUS - 1);
    }
#endif
  }
}

typedef struct {
  AppTimer *timer;
  int frame;
} LoadingData;

static void loading_update(Layer *layer, GContext *ctx) {
  LoadingData *data = layer_get_data(layer);
  GRect bounds = layer_get_bounds(layer);
  ui_draw_loading_dots(ctx, grect_center_point(&bounds), data->frame, false);
}

static void loading_tick(void *context) {
  Layer *layer = context;
  LoadingData *data = layer_get_data(layer);
  data->frame++;
  layer_mark_dirty(layer);
  data->timer = app_timer_register(UI_LOADING_FRAME_MS, loading_tick, layer);
}

Layer *ui_loading_layer_create(GRect frame) {
  Layer *layer = layer_create_with_data(frame, sizeof(LoadingData));
  LoadingData *data = layer_get_data(layer);
  data->timer = NULL;
  data->frame = 0;
  layer_set_update_proc(layer, loading_update);
  layer_set_hidden(layer, true);
  return layer;
}

void ui_loading_layer_set_active(Layer *layer, bool active) {
  LoadingData *data = layer_get_data(layer);
  layer_set_hidden(layer, !active);
  if (active && !data->timer) {
    data->frame = 0;
    data->timer = app_timer_register(UI_LOADING_FRAME_MS, loading_tick, layer);
  } else if (!active && data->timer) {
    app_timer_cancel(data->timer);
    data->timer = NULL;
  }
}

void ui_loading_layer_destroy(Layer *layer) {
  ui_loading_layer_set_active(layer, false);
  layer_destroy(layer);
}

// Vibration

// shorter than vibes_short_pulse() so the feedback is less intrusive
static const uint32_t SUCCESS_PATTERN[] = {50};
static const uint32_t FAILURE_PATTERN[] = {50, 120, 50};

void ui_vibrate(bool success) {
  if (!settings_get_vibrate()) {
    return;
  }
  VibePattern pattern =
      success ? (VibePattern){.durations = SUCCESS_PATTERN,
                              .num_segments = ARRAY_LENGTH(SUCCESS_PATTERN)}
              : (VibePattern){.durations = FAILURE_PATTERN,
                              .num_segments = ARRAY_LENGTH(FAILURE_PATTERN)};
  vibes_enqueue_custom_pattern(pattern);
}

// Text

void ui_copy_truncated(char *dst, const char *src, size_t dst_size) {
  size_t len = strlen(src);
  if (len >= dst_size) {
    len = dst_size - 1;
    // step back over UTF-8 continuation bytes so no character is split
    while (len > 0 && ((uint8_t)src[len] & 0xC0) == 0x80) {
      len--;
    }
  }
  memcpy(dst, src, len);
  dst[len] = '\0';
}

bool ui_is_blank(const char *text) {
  for (const char *c = text; *c; c++) {
    if (*c != ' ' && *c != '\n' && *c != '\t' && *c != '\r') {
      return false;
    }
  }
  return true;
}
