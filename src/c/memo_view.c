#include "memo_view.h"

#include "ui.h"

// two pages of lines; must match MAX_PAGE_LINES in index.js
#define MAX_PAGE_BLOCKS 60
#define MAX_BLOCKS (2 * MAX_PAGE_BLOCKS)
#define MAX_TEXT_HEIGHT 2000
#define BLOCK_SPACING 4
#define RULE_HEIGHT 9
#define BOTTOM_PADDING 10
// room for the loading dots where more lines can be loaded
#if PBL_DISPLAY_WIDTH >= 200
#define EDGE_HEIGHT 36
#else
#define EDGE_HEIGHT 30
#endif

typedef enum {
  BlockHeading1,
  BlockHeading2,
  BlockParagraph,
  BlockBullet,
  BlockTodo,
  BlockDone,
  BlockQuote,
  BlockTags,
  BlockCode,
  BlockRule,
  BlockMeta,
  BlockTypeCount,
} BlockType;

typedef struct {
  BlockType type;
  const char *text;
  int16_t y;
  int16_t height;
} Block;

struct MemoView {
  ScrollLayer *scroll_layer;
  Layer *content_layer;
  Layer *scroll_arrow;
  MemoViewEdgeHandler edge_handler;
  void *context;
  Block blocks[MAX_BLOCKS];
  int num_blocks;
  bool more_above;
  bool more_below;
  bool relayout;     // the offset changes, but not by scrolling
  int16_t last_top;  // to tell the direction of scrolling
  AppTimer *loading_timer;
  int loading_frame;
};

// Style

#if PBL_DISPLAY_WIDTH >= 200
#define FONT_HEADING1 FONT_KEY_GOTHIC_28_BOLD
#define FONT_HEADING2 FONT_KEY_GOTHIC_24_BOLD
#define FONT_BODY FONT_KEY_GOTHIC_24
#define FONT_SMALL FONT_KEY_GOTHIC_18
#define MARKER_WIDTH 20
#define CHECKBOX_SIZE 13
#else
#define FONT_HEADING1 FONT_KEY_GOTHIC_24_BOLD
#define FONT_HEADING2 FONT_KEY_GOTHIC_18_BOLD
#define FONT_BODY FONT_KEY_GOTHIC_18
#define FONT_SMALL FONT_KEY_GOTHIC_14
#define MARKER_WIDTH 15
#define CHECKBOX_SIZE 10
#endif

#define QUOTE_INDENT 8
#define MUTED_COLOR PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack)
#define TAG_COLOR PBL_IF_COLOR_ELSE(GColorCobaltBlue, GColorBlack)

static BlockType type_for_code(char code) {
  switch (code) {
    case '1':
      return BlockHeading1;
    case '2':
      return BlockHeading2;
    case 'b':
      return BlockBullet;
    case 'c':
      return BlockTodo;
    case 'x':
      return BlockDone;
    case 'q':
      return BlockQuote;
    case 't':
      return BlockTags;
    case 'k':
      return BlockCode;
    case '-':
      return BlockRule;
    case 'm':
      return BlockMeta;
    default:  // 'p', 'n' and anything unknown
      return BlockParagraph;
  }
}

static GFont font_for(BlockType type) {
  switch (type) {
    case BlockHeading1:
      return fonts_get_system_font(FONT_HEADING1);
    case BlockHeading2:
      return fonts_get_system_font(FONT_HEADING2);
    case BlockTags:
    case BlockCode:
    case BlockMeta:
      return fonts_get_system_font(FONT_SMALL);
    default:
      return fonts_get_system_font(FONT_BODY);
  }
}

static int16_t indent_for(BlockType type) {
  switch (type) {
    case BlockBullet:
    case BlockTodo:
    case BlockDone:
      return MARKER_WIDTH;
    case BlockQuote:
    case BlockCode:
      return QUOTE_INDENT;
    default:
      return 0;
  }
}

static GColor text_color_for(BlockType type) {
  switch (type) {
    case BlockDone:
    case BlockQuote:
    case BlockCode:
    case BlockMeta:
      return MUTED_COLOR;
    case BlockTags:
      return TAG_COLOR;
    default:
      return GColorBlack;
  }
}

// Layout

static int16_t text_height(const char *text, GFont font, int16_t width) {
  return graphics_text_layout_get_content_size(
             text, font, GRect(0, 0, width, MAX_TEXT_HEIGHT),
             GTextOverflowModeWordWrap, GTextAlignmentLeft)
      .h;
}

// Lays out the blocks. The block at anchor keeps its place on screen if
// it was at anchor_y before; with anchor -1 the view scrolls to the top.
static void layout(MemoView *view, int anchor, int16_t anchor_y) {
  GRect bounds = layer_get_bounds(scroll_layer_get_layer(view->scroll_layer));
  int16_t width = layer_get_bounds(view->content_layer).size.w;
  int16_t y = PBL_IF_ROUND_ELSE(bounds.size.h / 10, 2);
  if (view->more_above) {
    y += EDGE_HEIGHT;
  }

  for (int i = 0; i < view->num_blocks; i++) {
    Block *block = &view->blocks[i];
    block->y = y;
    if (block->type == BlockRule) {
      block->height = RULE_HEIGHT;
    } else {
      block->height = text_height(block->text, font_for(block->type),
                                  width - indent_for(block->type));
      // headings get a bit more room to separate sections
      if (block->type == BlockHeading1 || block->type == BlockHeading2) {
        block->height += BLOCK_SPACING;
      }
    }
    y += block->height + BLOCK_SPACING;
  }

  int16_t content_height =
      y + (view->more_below ? EDGE_HEIGHT : 0) + BOTTOM_PADDING;
  if (content_height < bounds.size.h) {
    content_height = bounds.size.h;
  }

  GPoint offset = GPointZero;
  if (anchor >= 0 && anchor < view->num_blocks) {
    offset = scroll_layer_get_content_offset(view->scroll_layer);
    offset.y -= view->blocks[anchor].y - anchor_y;
    int16_t min_y = bounds.size.h - content_height;
    offset.y = offset.y > 0 ? 0 : (offset.y < min_y ? min_y : offset.y);
  }

  view->relayout = true;
  GRect frame = layer_get_frame(view->content_layer);
  frame.size.h = content_height;
  layer_set_frame(view->content_layer, frame);
  scroll_layer_set_content_size(view->scroll_layer,
                                GSize(bounds.size.w, content_height));
  scroll_layer_set_content_offset(view->scroll_layer, offset, false);
  view->relayout = false;
  view->last_top = -scroll_layer_get_content_offset(view->scroll_layer).y;
  ui_scroll_arrow_update(view->scroll_arrow);
  layer_mark_dirty(view->content_layer);
}

// Lays out the blocks without moving the ones on screen, which start at
// first after the change and were at old_y before it
static void layout_keeping(MemoView *view, int first, int16_t old_y) {
  layout(view, view->num_blocks > 0 ? first : -1, old_y);
}

// Drawing

// vertical center of the first text line, for markers
static int16_t first_line_center(const Block *block) {
  // the fonts do not change, so each is measured once
  static int16_t s_line_heights[BlockTypeCount];
  int16_t *line_height = &s_line_heights[block->type];
  if (*line_height == 0) {
    *line_height = text_height("A", font_for(block->type), 100);
  }
  // the system fonts have extra space above the letters
  return block->y + *line_height / 2 + 3;
}

static void draw_checkbox(GContext *ctx, const Block *block, int16_t x) {
  int16_t center = first_line_center(block);
  GRect box =
      GRect(x + 1, center - CHECKBOX_SIZE / 2, CHECKBOX_SIZE, CHECKBOX_SIZE);

  graphics_context_set_stroke_color(
      ctx, block->type == BlockDone ? MUTED_COLOR : GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_round_rect(ctx, box, 2);

  if (block->type == BlockDone) {
    GPoint left = GPoint(box.origin.x + 2, box.origin.y + box.size.h / 2);
    GPoint bottom = GPoint(box.origin.x + box.size.w / 2 - 1,
                           box.origin.y + box.size.h - 3);
    GPoint right = GPoint(box.origin.x + box.size.w - 2, box.origin.y + 2);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, left, bottom);
    graphics_draw_line(ctx, bottom, right);
  }
}

static void draw_block(GContext *ctx, const Block *block, int16_t width) {
  int16_t indent = indent_for(block->type);

  switch (block->type) {
    case BlockRule:
      graphics_context_set_stroke_color(ctx, MUTED_COLOR);
      graphics_context_set_stroke_width(ctx, 1);
      graphics_draw_line(ctx, GPoint(0, block->y + RULE_HEIGHT / 2),
                         GPoint(width, block->y + RULE_HEIGHT / 2));
      return;
    case BlockBullet:
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_fill_circle(
          ctx, GPoint(MARKER_WIDTH / 3, first_line_center(block)), 2);
      break;
    case BlockTodo:
    case BlockDone:
      draw_checkbox(ctx, block, 0);
      break;
    case BlockQuote:
    case BlockCode:
      graphics_context_set_fill_color(
          ctx, PBL_IF_COLOR_ELSE(GColorLightGray, GColorBlack));
      graphics_fill_rect(ctx, GRect(0, block->y + 4, 3, block->height - 2), 0,
                         GCornerNone);
      break;
    default:
      break;
  }

  graphics_context_set_text_color(ctx, text_color_for(block->type));
  graphics_draw_text(ctx, block->text, font_for(block->type),
                     GRect(indent, block->y, width - indent, block->height),
                     GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

static void draw_edge_dots(GContext *ctx, const MemoView *view, int16_t y,
                           int16_t width) {
  ui_draw_loading_dots(ctx, GPoint(width / 2, y + EDGE_HEIGHT / 2),
                       view->loading_frame, false);
}

static void content_update(Layer *layer, GContext *ctx) {
  MemoView *view = *(MemoView **)layer_get_data(layer);
  int16_t width = layer_get_bounds(layer).size.w;

  if (view->more_above && view->num_blocks > 0) {
    draw_edge_dots(ctx, view, view->blocks[0].y - EDGE_HEIGHT, width);
  }
  if (view->more_below && view->num_blocks > 0) {
    const Block *last = &view->blocks[view->num_blocks - 1];
    draw_edge_dots(ctx, view, last->y + last->height + BLOCK_SPACING, width);
  }

  // only draw what is currently scrolled into view
  int16_t top = -scroll_layer_get_content_offset(view->scroll_layer).y;
  int16_t bottom =
      top + layer_get_bounds(scroll_layer_get_layer(view->scroll_layer)).size.h;

  for (int i = 0; i < view->num_blocks; i++) {
    const Block *block = &view->blocks[i];
    if (block->y > bottom) {
      break;
    }
    if (block->y + block->height >= top) {
      draw_block(ctx, block, width);
    }
  }
}

// Loading more lines

// More lines are loaded while there is still a screen of text to read.
// Only the edge the view is scrolled towards counts, so loading at one
// edge never makes the other one load right away.
void memo_view_check_edge(MemoView *view, bool at_end) {
  if (!view->edge_handler || view->num_blocks == 0) {
    return;
  }
  int16_t height =
      layer_get_bounds(scroll_layer_get_layer(view->scroll_layer)).size.h;
  int16_t top = -scroll_layer_get_content_offset(view->scroll_layer).y;
  int16_t content_height = scroll_layer_get_content_size(view->scroll_layer).h;
  if (at_end ? view->more_below && top + 2 * height >= content_height
             : view->more_above && top <= height) {
    view->edge_handler(at_end, view->context);
  }
}

static void offset_changed(ScrollLayer *scroll_layer, void *context) {
  MemoView *view = context;
  ui_scroll_arrow_update(view->scroll_arrow);
  int16_t top = -scroll_layer_get_content_offset(scroll_layer).y;
  if (view->relayout || top == view->last_top) {
    return;
  }
  bool down = top > view->last_top;
  view->last_top = top;
  memo_view_check_edge(view, down);
}

static void loading_tick(void *context) {
  MemoView *view = context;
  view->loading_frame++;
  layer_mark_dirty(view->content_layer);
  view->loading_timer =
      app_timer_register(UI_LOADING_FRAME_MS, loading_tick, view);
}

void memo_view_set_loading(MemoView *view, bool loading) {
  if (loading && !view->loading_timer) {
    view->loading_timer =
        app_timer_register(UI_LOADING_FRAME_MS, loading_tick, view);
  } else if (!loading && view->loading_timer) {
    app_timer_cancel(view->loading_timer);
    view->loading_timer = NULL;
  }
}

void memo_view_set_more(MemoView *view, bool above, bool below) {
  if (view->more_above == above && view->more_below == below) {
    return;
  }
  int16_t old_y = view->num_blocks > 0 ? view->blocks[0].y : 0;
  view->more_above = above;
  view->more_below = below;
  layout_keeping(view, 0, old_y);
}

// Public api

MemoView *memo_view_create(Layer *parent, GRect frame,
                           ClickConfigProvider click_config_provider,
                           Window *window, MemoViewEdgeHandler edge_handler,
                           void *context) {
  MemoView *view = calloc(1, sizeof(MemoView));
  view->edge_handler = edge_handler;
  view->context = context;

  view->scroll_layer = scroll_layer_create(frame);
  scroll_layer_set_context(view->scroll_layer, view);
  scroll_layer_set_callbacks(
      view->scroll_layer, (ScrollLayerCallbacks){
                              .click_config_provider = click_config_provider,
                              .content_offset_changed_handler = offset_changed,
                          });
  scroll_layer_set_click_config_onto_window(view->scroll_layer, window);

  // keep text away from the curved edges on round displays and clear of
  // the select hint on the right
  int16_t inset = PBL_IF_ROUND_ELSE(frame.size.w / 8, 4);
  view->content_layer = layer_create_with_data(
      GRect(inset, 0, frame.size.w - 2 * inset - UI_SELECT_HINT_RADIUS,
            frame.size.h),
      sizeof(MemoView *));
  *(MemoView **)layer_get_data(view->content_layer) = view;
  layer_set_update_proc(view->content_layer, content_update);

  scroll_layer_add_child(view->scroll_layer, view->content_layer);
  view->scroll_arrow = ui_scroll_arrow_create(view->scroll_layer);
  layer_add_child(parent, scroll_layer_get_layer(view->scroll_layer));
  return view;
}

void memo_view_destroy(MemoView *view) {
  memo_view_set_loading(view, false);
  layer_destroy(view->scroll_arrow);
  layer_destroy(view->content_layer);
  scroll_layer_destroy(view->scroll_layer);
  free(view);
}

// Reads the lines into blocks after the existing ones; returns how many
static int parse_lines(MemoView *view, char *lines) {
  int start = view->num_blocks;
  int limit = start + MAX_PAGE_BLOCKS;
  if (limit > MAX_BLOCKS) {
    limit = MAX_BLOCKS;
  }
  char *line = lines;
  while (line && *line && view->num_blocks < limit) {
    char *end = strchr(line, '\n');
    if (end) {
      *end = '\0';
    }
    Block *block = &view->blocks[view->num_blocks++];
    block->type = type_for_code(line[0]);
    block->text = line + 1;
    line = end ? end + 1 : NULL;
  }
  return view->num_blocks - start;
}

static void reverse_blocks(Block *blocks, int count) {
  for (int i = 0; i < count / 2; i++) {
    Block swap = blocks[i];
    blocks[i] = blocks[count - 1 - i];
    blocks[count - 1 - i] = swap;
  }
}

int memo_view_set_lines(MemoView *view, char *lines) {
  view->num_blocks = 0;
  view->more_above = view->more_below = false;
  int count = parse_lines(view, lines);
  layout(view, -1, 0);
  return count;
}

int memo_view_add_lines(MemoView *view, char *lines, bool at_end) {
  int old_count = view->num_blocks;
  int16_t old_y = old_count > 0 ? view->blocks[0].y : 0;
  int added = parse_lines(view, lines);
  if (!at_end) {
    // rotate the new blocks to the front
    reverse_blocks(view->blocks, view->num_blocks);
    reverse_blocks(view->blocks, added);
    reverse_blocks(view->blocks + added, old_count);
  }
  if (old_count == 0) {
    layout(view, -1, 0);
  } else {
    layout_keeping(view, at_end ? 0 : added, old_y);
  }
  return added;
}

void memo_view_remove_blocks(MemoView *view, int count, bool at_end) {
  if (count > view->num_blocks) {
    count = view->num_blocks;
  }
  int16_t old_y = 0;
  if (at_end) {
    old_y = view->num_blocks > 0 ? view->blocks[0].y : 0;
  } else {
    old_y = count < view->num_blocks ? view->blocks[count].y : 0;
    memmove(view->blocks, view->blocks + count,
            (view->num_blocks - count) * sizeof(Block));
  }
  view->num_blocks -= count;
  layout_keeping(view, 0, old_y);
}

void memo_view_set_message(MemoView *view, const char *message) {
  view->num_blocks = 1;
  view->more_above = view->more_below = false;
  view->blocks[0].type = BlockParagraph;
  view->blocks[0].text = message;
  layout(view, -1, 0);
}
