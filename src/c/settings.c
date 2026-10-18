#include "settings.h"

#define PERSIST_KEY_VIBRATE 50

static bool s_vibrate = true;
static int32_t s_glance_count = -1;

void settings_init(void) {
  if (persist_exists(PERSIST_KEY_VIBRATE)) {
    s_vibrate = persist_read_bool(PERSIST_KEY_VIBRATE);
  }
}

bool settings_get_vibrate(void) { return s_vibrate; }

void settings_set_vibrate(bool vibrate) {
  if (vibrate != s_vibrate || !persist_exists(PERSIST_KEY_VIBRATE)) {
    s_vibrate = vibrate;
    persist_write_bool(PERSIST_KEY_VIBRATE, vibrate);
  }
}

// not persisted: the glance itself is kept by the system
int32_t settings_get_glance_count(void) { return s_glance_count; }

void settings_set_glance_count(int32_t count) { s_glance_count = count; }
