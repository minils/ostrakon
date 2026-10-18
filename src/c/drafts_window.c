#include "drafts_window.h"

#include "actions.h"
#include "compose_window.h"
#include "drafts.h"
#include "i18n.h"
#include "list_window.h"
#include "toast.h"
#include "ui.h"

#define PREVIEW_SIZE 48

enum {
  DraftActionOpen = 1,
  DraftActionDelete,
};

static ListWindow *s_window;
static char s_previews[DRAFTS_MAX][PREVIEW_SIZE];
static int s_selected;

static void update_previews(void) {
  for (int i = 0; i < drafts_count(); i++) {
    drafts_read(i, s_previews[i], PREVIEW_SIZE);
  }
  list_window_reload(s_window);
}

// drafts change when one is sent or saved in the compose window
static void appear(void *context) { update_previews(); }

static uint16_t get_count(void *context) { return drafts_count(); }

static void get_row(uint16_t index, const char **title, const char **subtitle,
                    void *context) {
  *title = s_previews[index];
  *subtitle = NULL;
}

static void action_chosen(int id, void *context) {
  switch (id) {
    case DraftActionOpen:
      compose_window_push_draft(s_selected);
      break;
    case DraftActionDelete:
      drafts_delete(s_selected);
      update_previews();
      toast_show(tr("Draft deleted"), true);
      break;
  }
}

static void select_row(uint16_t index, void *context) {
  const ActionItem items[] = {
      {.label = tr("Open"), .id = DraftActionOpen},
      {.label = tr("Delete"),
       .id = DraftActionDelete,
       .confirm_label = tr("Yes, delete")},
  };
  s_selected = index;
  actions_open(items, ARRAY_LENGTH(items), action_chosen, NULL);
}

static void unload(void *context) { s_window = NULL; }

void drafts_window_push(void) {
  if (s_window) {
    return;
  }
  s_window = list_window_create(
      (ListWindowCallbacks){
          .get_count = get_count,
          .get_row = get_row,
          .select = select_row,
          .appear = appear,
          .unload = unload,
      },
      NULL);
  list_window_set_status(s_window, tr("No drafts"));
  list_window_push(s_window);
}
