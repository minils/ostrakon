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
  LanguageCount,  // the number of values, not a language
} Language;

void i18n_init(void);
void i18n_set_language(Language language);

// The language the texts are shown in: the setting, or for LanguageAuto
// the watch's language
Language i18n_language(void);

// Returns the translation of an English UI text for the current language,
// or the text itself. The translations are in resources/lang. The result stays
// valid for the whole app run.
const char *tr(const char *english);
