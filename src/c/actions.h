#pragma once
#include <pebble.h>

#define ACTIONS_MAX_ITEMS 9

typedef struct {
  const char *label;
  int id;  // must be > 0
  // if set, the action asks for confirmation with this label first
  const char *confirm_label;
} ActionItem;

// Called after the menu has closed with the id of the chosen action
typedef void (*ActionsCallback)(int id, void *context);

// Opens an action menu. Labels must stay valid while the menu is open.
void actions_open(const ActionItem *items, int count, ActionsCallback callback,
                  void *context);
