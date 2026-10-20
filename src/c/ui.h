#pragma once
#include <pebble.h>

/* ---------- Fonts ---------- */
// Larger displays (emery, gabbro) get bigger fonts so text is not tiny
#if PBL_DISPLAY_WIDTH >= 200
#define UI_TEXT_FONT_KEY FONT_KEY_GOTHIC_28
#else
#define UI_TEXT_FONT_KEY FONT_KEY_GOTHIC_24
#endif

/* ---------- Colors ---------- */
#define UI_HIGHLIGHT_COLOR PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack)
#define UI_HIGHLIGHT_TEXT_COLOR GColorWhite

/* ---------- Title bar and select hint ---------- */
#if PBL_DISPLAY_WIDTH >= 200
#define UI_TITLE_BAR_HEIGHT 20
#define UI_SELECT_HINT_RADIUS 7
#else
#define UI_TITLE_BAR_HEIGHT 16
#define UI_SELECT_HINT_RADIUS 5
#endif

// A bar at the top of the window showing a title. The title is not copied.
Layer *ui_title_bar_create(GRect window_bounds, const char *title);

// A notch at the right edge, next to the select button, showing that
// pressing select does something
Layer *ui_select_hint_create(GRect window_bounds);

// Three dots, one highlighted per animation frame, centered on center.
// on_highlight draws them for a highlighted menu row.
void ui_draw_loading_dots(GContext *ctx, GPoint center, int frame,
                          bool on_highlight);

// A layer showing the app's shard while active, with writing scratched
// onto it glyph by glyph, hidden otherwise
Layer *ui_loading_layer_create(GRect frame);
void ui_loading_layer_set_active(Layer *layer, bool active);
void ui_loading_layer_destroy(Layer *layer);

#define UI_LOADING_FRAME_MS 300

/* ---------- Status screens ---------- */
// What a status is about; it sets the color and icon of a status screen
typedef enum {
  UiStatusInfo,     // e.g. an empty list
  UiStatusNoPhone,  // a warning: the phone is not connected or silent
  UiStatusOffline,  // a warning: the phone cannot reach the server
  UiStatusError,    // a problem to fix, e.g. a rejected token
  UiStatusSuccess,  // a result, e.g. a memo was sent
} UiStatus;

// The icon PebbleOS shows in its dialogs for the same kind of problem, or
// NULL for none. Destroy it with gdraw_command_image_destroy().
GDrawCommandImage *ui_status_icon_create(UiStatus kind);

// Fills bounds like a Pebble dialog: the color of kind, and the icon and
// the wrapped text together in the middle
void ui_draw_status(GContext *ctx, GRect bounds, UiStatus kind,
                    GDrawCommandImage *icon, const char *text);

// Short, soft vibration if enabled in the settings
void ui_vibrate(bool success);

// Copies src into dst, cutting at a UTF-8 character boundary if needed
void ui_copy_truncated(char *dst, const char *src, size_t dst_size);

// True if text is empty or contains only whitespace
bool ui_is_blank(const char *text);
