#include <pebble.h>

#include "comm.h"
#include "compose_window.h"
#include "drafts.h"
#include "drafts_window.h"
#include "i18n.h"
#include "list_window.h"
#include "memo_list.h"
#include "search.h"
#include "settings.h"
#include "tag_list.h"

enum {
  MainRowNew,
  MainRowRecent,
  MainRowPinned,
  MainRowTags,
  MainRowSearch,
  MainRowDrafts,
  MainRowCount,
};

static ListWindow *s_main_window;
static char s_drafts_subtitle[16];

// Main menu

static uint16_t get_count(void *context) { return MainRowCount; }

static void get_row(uint16_t index, const char **title, const char **subtitle,
                    void *context) {
  switch (index) {
    case MainRowNew:
      *title = tr("New");
      *subtitle = tr("Dictate a memo");
      break;
    case MainRowRecent:
      *title = tr("Recent");
      *subtitle = tr("Latest memos");
      break;
    case MainRowPinned:
      *title = tr("Pinned");
      *subtitle = tr("Pinned memos");
      break;
    case MainRowTags:
      *title = tr("Tags");
      *subtitle = tr("Memos by tag");
      break;
    case MainRowSearch:
      *title = tr("Search");
      *subtitle = tr("Dictate words to find");
      break;
    case MainRowDrafts:
      *title = tr("Drafts");
      // formatted on every draw, so it follows a change of the language
      if (drafts_count() == 0) {
        snprintf(s_drafts_subtitle, sizeof(s_drafts_subtitle), "%s",
                 tr("None saved"));
      } else {
        snprintf(s_drafts_subtitle, sizeof(s_drafts_subtitle), tr("%d saved"),
                 drafts_count());
      }
      *subtitle = s_drafts_subtitle;
      break;
  }
}

static void select_row(uint16_t index, void *context) {
  switch (index) {
    case MainRowNew:
      compose_window_push_new();
      break;
    case MainRowRecent:
      memo_list_push_recent();
      break;
    case MainRowPinned:
      memo_list_push_pinned();
      break;
    case MainRowTags:
      tag_list_push();
      break;
    case MainRowSearch:
      search_start();
      break;
    case MainRowDrafts:
      drafts_window_push();
      break;
  }
}

// App lifecycle

static void init(void) {
  settings_init();
  i18n_init();
  comm_init();
  drafts_init();

#if PBL_API_EXISTS(app_touch_navigation_enable)
  // apps are left out of touch navigation unless they opt in; with it,
  // swipes scroll the lists and memos on watches with a touchscreen
  app_touch_navigation_enable(true);
#endif

  s_main_window = list_window_create(
      (ListWindowCallbacks){
          .get_count = get_count,
          .get_row = get_row,
          .select = select_row,
      },
      NULL);
  list_window_push(s_main_window);

  /* ---- Quick launch: start a new memo right away ---- */
  if (launch_reason() == APP_LAUNCH_QUICK_LAUNCH) {
    compose_window_push_new();
  }
}

/* ---- App glance: open todos in the launcher ---- */
#if PBL_API_EXISTS(app_glance_reload)
static char s_glance_text[32];

static void glance_reload(AppGlanceReloadSession *session, size_t limit,
                          void *context) {
  if (limit < 1) {
    return;
  }
  const AppGlanceSlice slice = {
      .layout =
          {
              .icon = APP_GLANCE_SLICE_DEFAULT_ICON,
              .subtitle_template_string = s_glance_text,
          },
      .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION,
  };
  app_glance_add_slice(session, slice);
}
#endif

static void deinit(void) {
#if PBL_API_EXISTS(app_glance_reload)
  // the phone sends the count while the app runs; keep the old glance
  // if it never arrived
  int32_t count = settings_get_glance_count();
  if (count >= 0) {
    if (count == 0) {
      snprintf(s_glance_text, sizeof(s_glance_text), "%s", tr("No open todos"));
    } else if (count == 1) {
      snprintf(s_glance_text, sizeof(s_glance_text), "%s", tr("1 open todo"));
    } else {
      snprintf(s_glance_text, sizeof(s_glance_text), tr("%d open todos"),
               (int)count);
    }
    app_glance_reload(glance_reload, NULL);
  }
#endif
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  // windows are destroyed when the app exits and the stack is cleared
}
