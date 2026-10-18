#pragma once
#include <pebble.h>

// Settings the watch needs without the phone, pushed from the config page

void settings_init(void);
bool settings_get_vibrate(void);
void settings_set_vibrate(bool vibrate);

// Open tasks of the todo tag for the app glance, -1 if not known yet
int32_t settings_get_glance_count(void);
void settings_set_glance_count(int32_t count);
