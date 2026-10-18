#include "memo_view.h"

#include "ui.h"

#define MAX_BLOCKS 80
#define MAX_TEXT_HEIGHT 2000
#define BLOCK_SPACING 4
#define RULE_HEIGHT 9
#define BOTTOM_PADDING 10

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
  Block blocks[MAX_BLOCKS];
  int num_blocks;
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

static void layout(MemoView *view) {
  GRect bounds = layer_get_bounds(scroll_layer_get_layer(view->scroll_layer));
  int16_t width = layer_get_bounds(view->content_layer).size.w;
  int16_t y = PBL_IF_ROUND_ELSE(bounds.size.h / 10, 2);

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

  int16_t content_height = y + BOTTOM_PADDING;
  if (content_height < bounds.size.h) {
    content_height = bounds.size.h;
  }

  GRect frame = layer_get_frame(view->content_layer);
  frame.size.h = content_height;
  layer_set_frame(view->content_layer, frame);
  scroll_layer_set_content_size(view->scroll_layer,
                                GSize(bounds.size.w, content_height));
  scroll_layer_set_content_offset(view->scroll_layer, GPointZero, false);
  layer_mark_dirty(view->content_layer);
}

// Drawing

// vertical center of the first text line, for markers
static int16_t first_line_center(const Block *block) {
  int16_t line_height = text_height("A", font_for(block->type), 100);
  // the system fonts have extra space above the letters
  return block->y + line_height / 2 + 3;
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

static void content_update(Layer *layer, GContext *ctx) {
  MemoView *view = *(MemoView **)layer_get_data(layer);
  int16_t width = layer_get_bounds(layer).size.w;

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

// Public api

MemoView *memo_view_create(Layer *parent, GRect frame,
                           ClickConfigProvider click_config_provider,
                           Window *window) {
  MemoView *view = calloc(1, sizeof(MemoView));

  view->scroll_layer = scroll_layer_create(frame);
  scroll_layer_set_callbacks(view->scroll_layer,
                             (ScrollLayerCallbacks){
                                 .click_config_provider = click_config_provider,
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
  layer_add_child(parent, scroll_layer_get_layer(view->scroll_layer));
  return view;
}

void memo_view_destroy(MemoView *view) {
  layer_destroy(view->content_layer);
  scroll_layer_destroy(view->scroll_layer);
  free(view);
}

void memo_view_set_lines(MemoView *view, char *lines) {
  view->num_blocks = 0;
  char *line = lines;
  while (line && *line && view->num_blocks < MAX_BLOCKS) {
    char *end = strchr(line, '\n');
    if (end) {
      *end = '\0';
    }
    Block *block = &view->blocks[view->num_blocks++];
    block->type = type_for_code(line[0]);
    block->text = line + 1;
    line = end ? end + 1 : NULL;
  }
  layout(view);
}

void memo_view_set_message(MemoView *view, const char *message) {
  view->num_blocks = 1;
  view->blocks[0].type = BlockParagraph;
  view->blocks[0].text = message;
  layout(view);
}
