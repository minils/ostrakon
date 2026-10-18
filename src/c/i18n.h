#pragma once
#include <pebble.h>

// Languages of the language setting (must match index.js)
typedef enum {
  LanguageAuto = 0,  // follows the watch's system language
  LanguageEnglish = 1,
  LanguageGerman = 2,
  LanguageFrench = 3,
  LanguageSpanish = 4,
  LanguageItalian = 5,
} Language;

void i18n_init(void);
void i18n_set_language(Language language);

// Returns the translation of an English UI text for the current language,
// or the text itself. The result stays valid for the whole app run.
const char *tr(const char *english);
