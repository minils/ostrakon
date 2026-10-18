#include "tasks_window.h"

#include "i18n.h"
#include "memo_window.h"
#include "remote_list.h"
#include "toast.h"
#include "ui.h"

#define TASKS_MAX 30
#define TASK_DONE 1

// rows fit two lines of text
#if PBL_DISPLAY_WIDTH >= 200
#define TASK_FONT FONT_KEY_GOTHIC_24
#define LINE_HEIGHT 26
#define CHECKBOX_SIZE 16
#else
#define TASK_FONT FONT_KEY_GOTHIC_18
#define LINE_HEIGHT 20
#define CHECKBOX_SIZE 12
#endif
#define CELL_HEIGHT (2 * LINE_HEIGHT + 6)

#define MUTED_COLOR PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)

static RemoteListItem s_items[TASKS_MAX];
static RemoteList s_list = {
    .items = s_items,
    .capacity = TASKS_MAX,
    .empty_text = "No tasks",
};
static char s_memo_name[COMM_ID_SIZE];
static int s_toggling = -1;
static bool s_changed;

// Drawing

static void draw_checkbox(GContext *ctx, GRect box, bool done, GColor color) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, box, 2);
  if (!done) {
    return;
  }
  GPoint left = GPoint(box.origin.x + 3, box.origin.y + box.size.h / 2);
  GPoint bottom =
      GPoint(box.origin.x + box.size.w / 2 - 1, box.origin.y + box.size.h - 4);
  GPoint right = GPoint(box.origin.x + box.size.w - 3, box.origin.y + 3);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_line(ctx, left, bottom);
  graphics_draw_line(ctx, bottom, right);
}

static void draw_row(GContext *ctx, const Layer *cell_layer, uint16_t index,
                     void *context) {
  const RemoteListItem *item = &s_items[index];
  GRect bounds = layer_get_bounds(cell_layer);
  bool done = item->flags & TASK_DONE;
  bool highlighted = menu_cell_layer_is_highlighted(cell_layer);

  GColor color = highlighted ? UI_HIGHLIGHT_TEXT_COLOR
                             : (done ? MUTED_COLOR : GColorBlack);
  int16_t x = PBL_IF_ROUND_ELSE(bounds.size.w / 8, 6);
  int16_t text_x = x + CHECKBOX_SIZE + 6;
  int16_t text_w = bounds.size.w - text_x - PBL_IF_ROUND_ELSE(x, 4);
  GFont font = fonts_get_system_font(TASK_FONT);

  // center short tasks vertically, longer ones use both lines
  int16_t text_h = graphics_text_layout_get_content_size(
                       item->title, font, GRect(0, 0, text_w, 2 * LINE_HEIGHT),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft)
                       .h;
  if (text_h > 2 * LINE_HEIGHT) {
    text_h = 2 * LINE_HEIGHT;
  }
  int16_t text_y = (bounds.size.h - text_h) / 2 - 3;

  // the checkbox sits next to the first line
  draw_checkbox(ctx,
                GRect(x, text_y + 3 + (LINE_HEIGHT - CHECKBOX_SIZE) / 2 + 1,
                      CHECKBOX_SIZE, CHECKBOX_SIZE),
                done, color);

  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, item->title, font,
                     GRect(text_x, text_y, text_w, 2 * LINE_HEIGHT + 4),
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft,
                     NULL);
}

// Toggling

static void toggle_handler(DictionaryIterator *iter, void *context) {
  comm_finish();
  int index = s_toggling;
  s_toggling = -1;
  if (index < 0 || index >= s_list.count) {
    return;
  }

  RemoteListItem *item = &s_items[index];
  if (!iter || comm_response_failed(iter)) {
    // undo the optimistic change
    item->flags ^= TASK_DONE;
    toast_show(iter ? comm_response_error(iter) : tr("No response from phone"),
               false);
  } else {
    item->flags = comm_get_int(iter, MESSAGE_KEY_ITEM_FLAGS, item->flags);
    s_changed = true;
    ui_vibrate(true);
  }
  list_window_reload(s_list.window);
}

static void select_row(uint16_t index, void *context) {
  // one change at a time, so a failure can be undone on the right task
  if (s_toggling >= 0) {
    return;
  }
  remote_list_cancel(&s_list);

  RemoteListItem *item = &s_items[index];
  // the title lets the phone check that the memo did not change meanwhile
  if (!comm_request(CommActionToggleTask, item->id, item->title, toggle_handler,
                    NULL)) {
    toast_show(tr("Phone not connected"), false);
    return;
  }
  s_toggling = index;
  item->flags ^= TASK_DONE;  // show the change right away
  list_window_reload(s_list.window);
}

// Window

static void unload(void *context) {
  remote_list_cancel(&s_list);
  // a pending toggle still completes on the phone, only the result is lost
  if (s_toggling >= 0) {
    comm_finish();
    s_toggling = -1;
    s_changed = true;
  }
  s_list.window = NULL;

  // the memo window below still shows the old checkboxes
  if (s_changed) {
    memo_window_reload(s_memo_name);
  }
}

void tasks_window_push(const char *memo_name) {
  if (s_list.window) {
    return;
  }
  ui_copy_truncated(s_memo_name, memo_name, sizeof(s_memo_name));
  s_changed = false;
  s_list.window = list_window_create(
      (ListWindowCallbacks){
          .get_count = remote_list_get_count,
          .get_row = remote_list_get_row,
          .select = select_row,
          .draw_row = draw_row,
          .cell_height = CELL_HEIGHT,
          .unload = unload,
      },
      &s_list);
  list_window_push(s_list.window);
  remote_list_load(&s_list, CommActionListTasks, s_memo_name, NULL);
}
