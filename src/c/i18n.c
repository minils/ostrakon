#include "i18n.h"

#define PERSIST_KEY_LANGUAGE 51

// The translations of one language, loaded from its resource
// (resources/lang/*.txt): lines of "English = translation". Only a hash
// of each English text is kept, so the table stays small; it is one
// allocation with the hashes, the pointers and the texts behind it.
typedef struct {
  int count;
  uint32_t *hashes;
  const char **texts;
} Table;

static Language s_setting = LanguageAuto;
static Language s_language = LanguageEnglish;
static Table *s_table;  // NULL for English

static Language system_language(void) {
  static const struct {
    const char *prefix;
    Language language;
  } LOCALES[] = {
      {"de", LanguageGerman},
      {"fr", LanguageFrench},
      {"es", LanguageSpanish},
      {"it", LanguageItalian},
  };
  const char *locale = i18n_get_system_locale();
  for (size_t i = 0; i < ARRAY_LENGTH(LOCALES); i++) {
    if (strncmp(locale, LOCALES[i].prefix, 2) == 0) {
      return LOCALES[i].language;
    }
  }
  return LanguageEnglish;
}

static uint32_t resource_id(Language language) {
  switch (language) {
    case LanguageGerman:
      return RESOURCE_ID_LANG_DE;
    case LanguageFrench:
      return RESOURCE_ID_LANG_FR;
    case LanguageSpanish:
      return RESOURCE_ID_LANG_ES;
    case LanguageItalian:
      return RESOURCE_ID_LANG_IT;
    default:
      return 0;
  }
}

// Turns the escape \n into a line break, in place
static void unescape(char *text) {
  char *out = text;
  for (char *in = text; *in; in++) {
    if (in[0] == '\\' && in[1] == 'n') {
      *out++ = '\n';
      in++;
    } else {
      *out++ = *in;
    }
  }
  *out = '\0';
}

// Splits the buffer into lines and the lines into English and translation;
// returns the number of translations
static int parse(char *buffer, const char **english, const char **texts,
                 int capacity) {
  int count = 0;
  char *line = buffer;
  while (*line) {
    char *end = strchr(line, '\n');
    char *next = end ? end + 1 : line + strlen(line);
    if (end) {
      *end = '\0';
      // a checkout with Windows line endings
      if (end > line && end[-1] == '\r') {
        end[-1] = '\0';
      }
    }
    char *separator = strstr(line, " = ");
    if (line[0] != '#' && separator && count < capacity) {
      *separator = '\0';
      unescape(line);
      unescape(separator + 3);
      english[count] = line;
      texts[count] = separator + 3;
      count++;
    }
    line = next;
  }
  return count;
}

// FNV-1a; test/i18n.test.js checks that no two texts share a hash
static uint32_t hash(const char *text) {
  uint32_t h = 2166136261u;
  for (const char *c = text; *c; c++) {
    h = (h ^ (uint8_t)*c) * 16777619u;
  }
  return h;
}

static Table *load(Language language) {
  uint32_t id = resource_id(language);
  if (!id) {
    return NULL;
  }
  ResHandle handle = resource_get_handle(id);
  size_t size = resource_size(handle);
  char *buffer = malloc(size + 1);
  if (!buffer) {
    return NULL;
  }
  resource_load(handle, (uint8_t *)buffer, size);
  buffer[size] = '\0';

  int lines = 1;
  for (char *c = buffer; *c; c++) {
    lines += *c == '\n';
  }
  const char **english = malloc(lines * sizeof(char *));
  const char **texts = malloc(lines * sizeof(char *));
  int count = english && texts ? parse(buffer, english, texts, lines) : 0;
  size_t text_size = 0;
  for (int i = 0; i < count; i++) {
    text_size += strlen(texts[i]) + 1;
  }
  Table *table =
      count ? malloc(sizeof(Table) +
                     count * (sizeof(uint32_t) + sizeof(char *)) + text_size)
            : NULL;
  if (table) {
    table->count = count;
    table->texts = (const char **)(table + 1);
    table->hashes = (uint32_t *)(table->texts + count);
    char *out = (char *)(table->hashes + count);
    for (int i = 0; i < count; i++) {
      size_t len = strlen(texts[i]) + 1;
      memcpy(out, texts[i], len);
      table->texts[i] = out;
      table->hashes[i] = hash(english[i]);
      out += len;
    }
  }
  free(english);
  free(texts);
  free(buffer);
  return table;
}

static void update(void) {
  Language language = s_setting == LanguageAuto ? system_language() : s_setting;
  if (language == s_language) {
    return;
  }
  s_language = language;
  // the old table is not freed: texts already handed out, e.g. to an open
  // menu, stay valid for the whole app run
  s_table = load(language);
}

void i18n_init(void) {
  if (persist_exists(PERSIST_KEY_LANGUAGE)) {
    s_setting = persist_read_int(PERSIST_KEY_LANGUAGE);
  }
  update();
}

void i18n_set_language(Language language) {
  if (language != s_setting || !persist_exists(PERSIST_KEY_LANGUAGE)) {
    s_setting = language;
    persist_write_int(PERSIST_KEY_LANGUAGE, language);
  }
  update();
}

const char *tr(const char *english) {
  if (!s_table) {
    return english;
  }
  uint32_t h = hash(english);
  for (int i = 0; i < s_table->count; i++) {
    if (s_table->hashes[i] == h) {
      return s_table->texts[i];
    }
  }
  return english;
}
