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

// Scroll arrow, sized and placed like the one of the notifications

#ifdef PBL_ROUND
#define SCROLL_ARROW_HEIGHT 16
#else
#define SCROLL_ARROW_HEIGHT 19
#endif
#define SCROLL_ARROW_WIDTH 12

static void scroll_arrow_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
#ifdef PBL_ROUND
  int16_t top = bounds.size.h - SCROLL_ARROW_WIDTH / 2 - 8;
#else
  // a strip in the background color keeps the text clear of the arrow
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  int16_t top = 7;
#endif
  // a triangle pointing down, one row at a time
  graphics_context_set_fill_color(ctx, GColorBlack);
  int16_t left = (bounds.size.w - SCROLL_ARROW_WIDTH) / 2;
  for (int16_t row = 0; row < SCROLL_ARROW_WIDTH / 2; row++) {
    graphics_fill_rect(
        ctx, GRect(left + row, top + row, SCROLL_ARROW_WIDTH - 2 * row, 1), 0,
        GCornerNone);
  }
}

Layer *ui_scroll_arrow_create(ScrollLayer *scroll_layer) {
  scroll_layer_set_shadow_hidden(scroll_layer, true);
  Layer *parent = scroll_layer_get_layer(scroll_layer);
  GRect bounds = layer_get_bounds(parent);
  Layer *layer =
      layer_create_with_data(GRect(0, bounds.size.h - SCROLL_ARROW_HEIGHT,
                                   bounds.size.w, SCROLL_ARROW_HEIGHT),
                             sizeof(ScrollLayer *));
  *(ScrollLayer **)layer_get_data(layer) = scroll_layer;
  layer_set_update_proc(layer, scroll_arrow_update);
  // above the content, which scrolls below it
  layer_add_child(parent, layer);
  ui_scroll_arrow_update(layer);
  return layer;
}

void ui_scroll_arrow_update(Layer *arrow) {
  ScrollLayer *scroll_layer = *(ScrollLayer **)layer_get_data(arrow);
  int16_t bottom =
      -scroll_layer_get_content_offset(scroll_layer).y +
      layer_get_bounds(scroll_layer_get_layer(scroll_layer)).size.h;
  layer_set_hidden(arrow,
                   bottom >= scroll_layer_get_content_size(scroll_layer).h);
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

// The app icon's shard, drawn like the PebbleOS icons: a flat fill with a
// thick black outline. Coordinates are in tenths of a 50x50 grid, twice
// the 25x25 grid of the shard in store/make_art.py.
//
// It moves like the icons of the PebbleOS Weather app: the shard drops in
// with a squash and stretch and floats gently, while the writing is
// scratched onto it stroke by stroke with dust flying off the tip. Once
// the writing is done the shard hops, shakes the writing off and it starts
// over.

#define SHARD_GRID 500
#define SHARD_NUM_POINTS 11
static const GPoint SHARD_POINTS[SHARD_NUM_POINTS] = {
    {30, 150},  {110, 70},  {290, 70},  {330, 120}, {380, 100}, {390, 150},
    {460, 180}, {460, 300}, {360, 410}, {130, 410}, {30, 310}};

// the writing, one horizontal stroke per entry, scratched in this order
typedef struct {
  int16_t from_x;
  int16_t to_x;
  int16_t y;
} Stroke;

static const Stroke WRITING[] = {
    {110, 280, 160}, {110, 180, 230}, {220, 380, 230},
    {110, 240, 300}, {280, 360, 300},
};
#define NUM_STROKES ((int)ARRAY_LENGTH(WRITING))

// The shard squashes and stretches around the middle of its bottom edge
#define SHARD_ANCHOR_X 250
#define SHARD_ANCHOR_Y 410

// Where the shard is and how it is squashed, per frame: an offset in
// tenths of the grid and a scale in percent
typedef struct {
  int16_t dy;
  uint8_t scale_x;
  uint8_t scale_y;
} Pose;

// falls in stretched, lands squashed and bounces back into shape
static const Pose DROP_IN[] = {
    {-250, 80, 125}, {-150, 85, 120}, {-60, 92, 110}, {0, 120, 80},
    {0, 110, 90},    {-30, 95, 106},  {-10, 102, 98}, {0, 100, 100},
};
// crouches, hops and lands again while the writing is shaken off
static const Pose HOP[] = {
    {0, 108, 92}, {-40, 94, 108}, {-60, 96, 105}, {-40, 100, 100},
    {0, 114, 86}, {0, 104, 96},   {0, 100, 100},
};
#define DROP_IN_FRAMES ((int)ARRAY_LENGTH(DROP_IN))
#define HOP_FRAMES ((int)ARRAY_LENGTH(HOP))
// frames the writing takes to vanish at the start of the hop
#define WIPE_FRAMES 3

// grid tenths written per frame, and the pause before each stroke
#define WRITE_SPEED 45
#define STROKE_GAP 60
// frames the finished writing stays before the hop
#define HOLD_FRAMES 12
// frames of one float up and down
#define FLOAT_FRAMES 40
#define LOADING_SHARD_FRAME_MS 66

#ifdef PBL_COLOR
#define SHARD_FILL_COLOR GColorOrange
#else
#define SHARD_FILL_COLOR GColorWhite
#endif

typedef enum {
  LoadingDropIn,
  LoadingWrite,
  LoadingHold,
  LoadingHop,
} LoadingStage;

typedef struct {
  AppTimer *timer;
  LoadingStage stage;
  int stage_frame;
  int frame;  // since the shard landed, for floating
  GPath *shard_path;
  GPoint shard_points[SHARD_NUM_POINTS];
  GPoint origin;
  int size;
} LoadingData;

// a point of the grid on the screen, floated up or down by dy (in grid
// tenths) and squashed by pose
static GPoint shard_point(const LoadingData *data, GPoint grid, int dy,
                          const Pose *pose) {
  int32_t x = SHARD_ANCHOR_X + (grid.x - SHARD_ANCHOR_X) * pose->scale_x / 100;
  int32_t y = SHARD_ANCHOR_Y + (grid.y - SHARD_ANCHOR_Y) * pose->scale_y / 100 +
              pose->dy + dy;
  return GPoint(data->origin.x + x * data->size / SHARD_GRID,
                data->origin.y + y * data->size / SHARD_GRID);
}

// amplitude * sin of frame out of period frames
static int wave(int amplitude, int frame, int period, int32_t offset) {
  int32_t angle = (frame % period) * TRIG_MAX_ANGLE / period + offset;
  return amplitude * sin_lookup(angle) / TRIG_MAX_RATIO;
}

static int writing_length(void) {
  int length = 0;
  for (int i = 0; i < NUM_STROKES; i++) {
    length += STROKE_GAP + WRITING[i].to_x - WRITING[i].from_x;
  }
  return length;
}

static void loading_update(Layer *layer, GContext *ctx) {
  LoadingData *data = layer_get_data(layer);
  static const Pose rest = {0, 100, 100};
  const Pose *pose = &rest;
  if (data->stage == LoadingDropIn) {
    pose = &DROP_IN[data->stage_frame];
  } else if (data->stage == LoadingHop) {
    pose = &HOP[data->stage_frame];
  }
  int outline = data->size >= 100 ? 5 : 3;
  int float_y =
      data->stage == LoadingDropIn ? 0 : wave(15, data->frame, FLOAT_FRAMES, 0);

  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_stroke_color(ctx, GColorBlack);

  for (int i = 0; i < SHARD_NUM_POINTS; i++) {
    data->shard_points[i] = shard_point(data, SHARD_POINTS[i], float_y, pose);
  }
  graphics_context_set_fill_color(ctx, SHARD_FILL_COLOR);
  gpath_draw_filled(ctx, data->shard_path);
  graphics_context_set_stroke_width(ctx, outline);
  gpath_draw_outline(ctx, data->shard_path);

  // how much of the writing is there: all of it until the hop shakes it off
  int written = 0;
  if (data->stage == LoadingWrite) {
    written = data->stage_frame * WRITE_SPEED;
  } else if (data->stage == LoadingHold ||
             (data->stage == LoadingHop && data->stage_frame < WIPE_FRAMES)) {
    written = writing_length();
  }
  // while it is shaken off, each stroke shrinks toward its middle
  int shrink = data->stage == LoadingHop ? data->stage_frame : 0;
  graphics_context_set_stroke_width(ctx, outline > 3 ? 4 : 3);
  for (int i = 0; i < NUM_STROKES && written > STROKE_GAP; i++) {
    written -= STROKE_GAP;
    const Stroke *stroke = &WRITING[i];
    int length = stroke->to_x - stroke->from_x;
    int from_x = stroke->from_x;
    int to_x = stroke->from_x + (written < length ? written : length);
    if (shrink) {
      int cut = length * shrink / (2 * WIPE_FRAMES);
      from_x += cut;
      to_x -= cut;
    }
    graphics_draw_line(
        ctx, shard_point(data, GPoint(from_x, stroke->y), float_y, pose),
        shard_point(data, GPoint(to_x, stroke->y), float_y, pose));
    if (written < length) {
      // dust flying off the tip, a little further every frame
      GPoint tip = GPoint(to_x, stroke->y);
      int fly = 20 + 15 * (data->stage_frame % 3);
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_fill_circle(
          ctx,
          shard_point(data, GPoint(tip.x + fly, tip.y - 2 * fly), float_y,
                      pose),
          1);
      graphics_fill_circle(
          ctx,
          shard_point(data, GPoint(tip.x + 2 * fly, tip.y - fly), float_y,
                      pose),
          1);
    }
    written -= length;
  }
}

static void loading_tick(void *context) {
  Layer *layer = context;
  LoadingData *data = layer_get_data(layer);
  data->stage_frame++;
  if (data->stage != LoadingDropIn) {
    data->frame++;
  }
  switch (data->stage) {
    case LoadingDropIn:
      if (data->stage_frame >= DROP_IN_FRAMES) {
        data->stage = LoadingWrite;
        data->stage_frame = 0;
      }
      break;
    case LoadingWrite:
      if (data->stage_frame * WRITE_SPEED >= writing_length()) {
        data->stage = LoadingHold;
        data->stage_frame = 0;
      }
      break;
    case LoadingHold:
      if (data->stage_frame >= HOLD_FRAMES) {
        data->stage = LoadingHop;
        data->stage_frame = 0;
      }
      break;
    case LoadingHop:
      if (data->stage_frame >= HOP_FRAMES) {
        data->stage = LoadingWrite;
        data->stage_frame = 0;
      }
      break;
  }
  layer_mark_dirty(layer);
  data->timer = app_timer_register(LOADING_SHARD_FRAME_MS, loading_tick, layer);
}

Layer *ui_loading_layer_create(GRect frame) {
  Layer *layer = layer_create_with_data(frame, sizeof(LoadingData));
  LoadingData *data = layer_get_data(layer);
  data->timer = NULL;
  data->stage = LoadingDropIn;
  data->stage_frame = 0;
  data->frame = 0;

  // three fifths of the shorter side, so the shard fits round displays too
  data->size =
      (frame.size.w < frame.size.h ? frame.size.w : frame.size.h) * 3 / 5;
  data->origin =
      GPoint((frame.size.w - data->size) / 2, (frame.size.h - data->size) / 2);
  // the points are filled in when drawing
  data->shard_path = gpath_create(&(GPathInfo){.num_points = SHARD_NUM_POINTS,
                                               .points = data->shard_points});

  layer_set_update_proc(layer, loading_update);
  layer_set_hidden(layer, true);
  return layer;
}

void ui_loading_layer_set_active(Layer *layer, bool active) {
  LoadingData *data = layer_get_data(layer);
  layer_set_hidden(layer, !active);
  if (active && !data->timer) {
    data->stage = LoadingDropIn;
    data->stage_frame = 0;
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
  LoadingData *data = layer_get_data(layer);
  gpath_destroy(data->shard_path);
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

// Status screens
// The layout of simple_dialog in PebbleOS

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
#define STATUS_SUCCESS_COLOR PBL_IF_COLOR_ELSE(GColorJaegerGreen, GColorWhite)
#define STATUS_LIGHT_TEXT_COLOR PBL_IF_COLOR_ELSE(GColorWhite, GColorBlack)

GDrawCommandImage *ui_status_icon_create(UiStatus kind) {
  switch (kind) {
    case UiStatusNoPhone:
      return gdraw_command_image_create_with_resource(
          RESOURCE_ID_ICON_NO_PHONE);
    case UiStatusOffline:
      return gdraw_command_image_create_with_resource(
          RESOURCE_ID_ICON_NO_INTERNET);
    case UiStatusError:
      return gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_ERROR);
    case UiStatusSuccess:
      return gdraw_command_image_create_with_resource(RESOURCE_ID_ICON_SUCCESS);
    default:
      return NULL;
  }
}

void ui_draw_status(GContext *ctx, GRect bounds, UiStatus kind,
                    GDrawCommandImage *icon, const char *text) {
  GColor color = STATUS_INFO_COLOR;
  GColor text_color = STATUS_LIGHT_TEXT_COLOR;
  if (kind == UiStatusNoPhone || kind == UiStatusOffline) {
    color = STATUS_WARNING_COLOR;
    text_color = GColorBlack;
  } else if (kind == UiStatusError) {
    color = STATUS_ERROR_COLOR;
  } else if (kind == UiStatusSuccess) {
    color = STATUS_SUCCESS_COLOR;
    text_color = GColorBlack;
  }
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  GFont font = fonts_get_system_font(STATUS_FONT_KEY);
  GRect text_box = grect_inset(bounds, GEdgeInsets(0, STATUS_INSET));
  GSize text_size = graphics_text_layout_get_content_size(
      text, font, text_box, GTextOverflowModeWordWrap, GTextAlignmentCenter);
  GSize icon_size =
      icon ? gdraw_command_image_get_bounds_size(icon) : GSizeZero;
  int16_t spacing = icon ? STATUS_ICON_SPACING : 0;

  int16_t height = icon_size.h + spacing + text_size.h;
  int16_t top = bounds.origin.y +
                (height < bounds.size.h ? (bounds.size.h - height) / 2 : 0);
  if (icon) {
    gdraw_command_image_draw(
        ctx, icon,
        GPoint(bounds.origin.x + (bounds.size.w - icon_size.w) / 2, top));
  }
  text_box.origin.y = top + icon_size.h + spacing - STATUS_FONT_TOP;
  text_box.size.h = bounds.origin.y + bounds.size.h - text_box.origin.y;
  graphics_context_set_text_color(ctx, text_color);
  graphics_draw_text(ctx, text, font, text_box, GTextOverflowModeWordWrap,
                     GTextAlignmentCenter, NULL);
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
