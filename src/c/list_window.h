#pragma once
#include <pebble.h>

typedef struct ListWindow ListWindow;

typedef struct {
  uint16_t (*get_count)(void *context);
  void (*get_row)(uint16_t index, const char **title, const char **subtitle,
                  void *context);
  void (*select)(uint16_t index, void *context);
  void (*select_long)(uint16_t index, void *context);  // optional
  // optional, replaces the default title/subtitle drawing of item rows
  void (*draw_row)(GContext *ctx, const Layer *cell_layer, uint16_t index,
                   void *context);
  int16_t cell_height;            // optional, 0 for the default
  void (*appear)(void *context);  // optional
  void (*unload)(void *context);  // optional, the ListWindow is freed after
} ListWindowCallbacks;

// A menu window. When there are no rows, the status text is shown instead.
ListWindow *list_window_create(ListWindowCallbacks callbacks, void *context);
void list_window_push(ListWindow *list);
void list_window_remove(ListWindow *list);
void list_window_reload(ListWindow *list);
void list_window_set_status(ListWindow *list, const char *status);

// Shows a title bar above the rows; set before pushing the window
void list_window_set_title(ListWindow *list, const char *title);

// Shows the loading shard instead of the status, or loading dots after the rows
void list_window_set_loading(ListWindow *list, bool loading);
