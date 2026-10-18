#include "actions.h"

#include "i18n.h"
#include "ui.h"

#define ACTIONS_CANCEL (-1)

static ActionsCallback s_callback;
static void *s_context;

static void action_performed(ActionMenu *menu, const ActionMenuItem *action,
                             void *context) {
  // handled in menu_did_close, once the menu is gone
}

static void menu_did_close(ActionMenu *menu, const ActionMenuItem *performed,
                           void *context) {
  int id = performed
               ? (int)(intptr_t)action_menu_item_get_action_data(performed)
               : 0;
  action_menu_hierarchy_destroy(action_menu_get_root_level(menu), NULL, NULL);

  if (id > 0 && s_callback) {
    s_callback(id, s_context);
  }
}

void actions_open(const ActionItem *items, int count, ActionsCallback callback,
                  void *context) {
  s_callback = callback;
  s_context = context;

  ActionMenuLevel *root = action_menu_level_create(count);
  for (int i = 0; i < count; i++) {
    void *data = (void *)(intptr_t)items[i].id;
    if (items[i].confirm_label) {
      ActionMenuLevel *confirm = action_menu_level_create(2);
      action_menu_level_add_action(confirm, items[i].confirm_label,
                                   action_performed, data);
      action_menu_level_add_action(confirm, tr("Cancel"), action_performed,
                                   (void *)(intptr_t)ACTIONS_CANCEL);
      action_menu_level_add_child(root, confirm, items[i].label);
    } else {
      action_menu_level_add_action(root, items[i].label, action_performed,
                                   data);
    }
  }

  ActionMenuConfig config = (ActionMenuConfig){
      .root_level = root,
      .colors =
          {
              .background = UI_HIGHLIGHT_COLOR,
              .foreground = UI_HIGHLIGHT_TEXT_COLOR,
          },
      .align = ActionMenuAlignCenter,
      .did_close = menu_did_close,
  };
  action_menu_open(&config);
}
