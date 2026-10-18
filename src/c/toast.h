#pragma once
#include <pebble.h>

// Shows a short message in its own window, which closes after a moment
void toast_show(const char *text, bool success);

// Same without vibration, for information that is not a result
void toast_show_quiet(const char *text);
