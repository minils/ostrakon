#pragma once
#include <pebble.h>

// Receives the dictated text; it is never blank
typedef void (*DictationHandler)(const char *text);

// A reusable dictation, kept in a static variable by its user
typedef struct {
  DictationHandler handler;
  // whether the system shows the text for confirmation first
  bool confirm;
  DictationSession *session;
} Dictation;

// Starts listening; failures other than a user cancel show a toast
void dictation_start(Dictation *dictation);

// Frees the session; the next start creates a new one
void dictation_release(Dictation *dictation);

// Adds text to a buffer, separated from what is there by a space.
// Leaves the buffer unchanged and returns false if it does not fit.
bool dictation_join(char *buffer, size_t size, const char *text);
