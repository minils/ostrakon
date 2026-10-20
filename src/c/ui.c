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

// The app icon's shard, on which the writing is scratched glyph by glyph.
// Coordinates are in a 50x50 grid, twice that of the shard in
// store/make_art.py.

#define SHARD_GRID 50
#define SHARD_NUM_POINTS 16
static const GPoint SHARD_POINTS[SHARD_NUM_POINTS] = {
    {1, 14},  {9, 7},   {16, 8},  {25, 3},  {35, 6},  {49, 10},
    {46, 19}, {49, 26}, {42, 35}, {44, 41}, {32, 47}, {26, 42},
    {17, 45}, {12, 37}, {4, 34},  {6, 24}};

// made-up glyphs of one or two strokes in a 3x5 box
typedef struct {
  GPoint from[2];
  GPoint to[2];
  int num_strokes;
} Glyph;

static const Glyph GLYPHS[] = {
    {{{0, 5}}, {{2, 0}}, 1},                  // slash
    {{{0, 0}, {0, 4}}, {{0, 4}, {3, 3}}, 2},  // hook
    {{{0, 1}, {1, 0}}, {{3, 1}, {1, 5}}, 2},  // cross
    {{{0, 5}, {1, 0}}, {{1, 0}, {3, 5}}, 2},  // lambda
    {{{0, 2}, {3, 2}}, {{3, 2}, {1, 5}}, 2},  // seven
    {{{0, 0}, {0, 5}}, {{3, 5}, {1, 3}}, 2},  // tick
};

// one word per entry: where it starts and which glyphs it has
typedef struct {
  GPoint origin;
  int num_glyphs;
  uint8_t glyphs[4];
} Word;

#define GLYPH_ADVANCE 4
static const Word WORDS[] = {
    {{8, 11}, 4, {1, 3, 0, 4}}, {{26, 11}, 4, {2, 5, 1, 0}},
    {{10, 20}, 3, {4, 0, 3}},   {{24, 20}, 4, {5, 1, 2, 3}},
    {{15, 29}, 4, {0, 4, 5, 1}},
};
#define NUM_WORDS ((int)ARRAY_LENGTH(WORDS))
#define NUM_WRITTEN_GLYPHS 19
// frames the finished writing stays before it starts over
#define LOADING_HOLD_FRAMES 8
#define LOADING_SHARD_FRAME_MS 100

#ifdef PBL_COLOR
#define SHARD_FILL_COLOR GColorRajah
#define SHARD_EDGE_COLOR GColorWindsorTan
#define SHARD_INK_COLOR GColorBulgarianRose
#else
#define SHARD_FILL_COLOR GColorWhite
#define SHARD_EDGE_COLOR GColorBlack
#define SHARD_INK_COLOR GColorBlack
#endif

typedef struct {
  AppTimer *timer;
  int frame;
  GPath *path;
  GPoint points[SHARD_NUM_POINTS];
  GPoint origin;
  int size;
} LoadingData;

static GPoint shard_point(const LoadingData *data, GPoint grid) {
  return GPoint(data->origin.x + grid.x * data->size / SHARD_GRID,
                data->origin.y + grid.y * data->size / SHARD_GRID);
}

static void loading_update(Layer *layer, GContext *ctx) {
  LoadingData *data = layer_get_data(layer);

  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, SHARD_FILL_COLOR);
  gpath_draw_filled(ctx, data->path);
  graphics_context_set_stroke_color(ctx, SHARD_EDGE_COLOR);
  graphics_context_set_stroke_width(ctx, 2);
  gpath_draw_outline(ctx, data->path);

  int written = data->frame % (NUM_WRITTEN_GLYPHS + LOADING_HOLD_FRAMES);
  graphics_context_set_stroke_color(ctx, SHARD_INK_COLOR);
  graphics_context_set_stroke_width(ctx, data->size >= 90 ? 3 : 2);
  int count = 0;
  for (int w = 0; w < NUM_WORDS; w++) {
    const Word *word = &WORDS[w];
    for (int g = 0; g < word->num_glyphs; g++) {
      if (count++ >= written) {
        return;
      }
      const Glyph *glyph = &GLYPHS[word->glyphs[g]];
      GPoint at = GPoint(word->origin.x + g * GLYPH_ADVANCE, word->origin.y);
      for (int s = 0; s < glyph->num_strokes; s++) {
        GPoint from = GPoint(at.x + glyph->from[s].x, at.y + glyph->from[s].y);
        GPoint to = GPoint(at.x + glyph->to[s].x, at.y + glyph->to[s].y);
        graphics_draw_line(ctx, shard_point(data, from),
                           shard_point(data, to));
      }
    }
  }
}

static void loading_tick(void *context) {
  Layer *layer = context;
  LoadingData *data = layer_get_data(layer);
  data->frame++;
  layer_mark_dirty(layer);
  data->timer =
      app_timer_register(LOADING_SHARD_FRAME_MS, loading_tick, layer);
}

Layer *ui_loading_layer_create(GRect frame) {
  Layer *layer = layer_create_with_data(frame, sizeof(LoadingData));
  LoadingData *data = layer_get_data(layer);
  data->timer = NULL;
  data->frame = 0;

  // half the shorter side, so the shard fits round displays too
  data->size = (frame.size.w < frame.size.h ? frame.size.w : frame.size.h) / 2;
  data->origin = GPoint((frame.size.w - data->size) / 2,
                        (frame.size.h - data->size) / 2);
  for (int i = 0; i < SHARD_NUM_POINTS; i++) {
    data->points[i] = shard_point(data, SHARD_POINTS[i]);
  }
  GPathInfo info = {.num_points = SHARD_NUM_POINTS, .points = data->points};
  data->path = gpath_create(&info);

  layer_set_update_proc(layer, loading_update);
  layer_set_hidden(layer, true);
  return layer;
}

void ui_loading_layer_set_active(Layer *layer, bool active) {
  LoadingData *data = layer_get_data(layer);
  layer_set_hidden(layer, !active);
  if (active && !data->timer) {
    data->frame = 0;
    data->timer =
        app_timer_register(LOADING_SHARD_FRAME_MS, loading_tick, layer);
  } else if (!active && data->timer) {
    app_timer_cancel(data->timer);
    data->timer = NULL;
  }
}

void ui_loading_layer_destroy(Layer *layer) {
  ui_loading_layer_set_active(layer, false);
  gpath_destroy(((LoadingData *)layer_get_data(layer))->path);
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
