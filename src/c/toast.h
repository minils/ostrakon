#pragma once
#include <pebble.h>

#include "ui.h"

// Shows a short message in its own window, which closes after a moment
void toast_show(const char *text, bool success);

// Same as a status screen (color, icon and text), without vibration; a
// caller showing a result vibrates itself
void toast_show_status(const char *text, UiStatus kind);
