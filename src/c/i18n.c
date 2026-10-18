#include "i18n.h"

#define PERSIST_KEY_LANGUAGE 51

typedef struct {
  const char *english;
  const char *german;
} Translation;

static const Translation GERMAN[] = {
    // main menu
    {"New", "Neu"},
    {"Dictate a memo", "Memo diktieren"},
    {"Recent", "Neueste"},
    {"Latest memos", "Letzte Memos"},
    {"Pinned", "Angepinnt"},
    {"Pinned memos", "Angepinnte Memos"},
    {"Tags", "Tags"},
    {"Memos by tag", "Memos nach Tag"},
    {"Search", "Suche"},
    {"Dictate words to find", "Suchbegriff diktieren"},
    {"Drafts", "Entwürfe"},
    {"None saved", "Keine gespeichert"},
    {"%d saved", "%d gespeichert"},
    // title bars
    {"Draft", "Entwurf"},
    {"Comment", "Kommentar"},
    {"Append", "Anhängen"},
    {"Memo", "Memo"},
    {"Comments", "Kommentare"},
    // actions
    {"Send", "Senden"},
    {"Dictate more", "Weiter diktieren"},
    {"Discard", "Verwerfen"},
    {"Yes, discard", "Ja, verwerfen"},
    {"Open", "Öffnen"},
    {"Tasks", "Aufgaben"},
    {"New comment", "Neuer Kommentar"},
    {"Pin / Unpin", "Anpinnen / Lösen"},
    {"Archive", "Archivieren"},
    {"Delete", "Löschen"},
    {"Yes, delete", "Ja, löschen"},
    {"Cancel", "Abbrechen"},
    // results
    {"Archived", "Archiviert"},
    {"Deleted", "Gelöscht"},
    {"Sent", "Gesendet"},
    {"Comment added", "Kommentar hinzugefügt"},
    {"Appended", "Angehängt"},
    {"Comment discarded", "Kommentar verworfen"},
    {"Append discarded", "Anhängen verworfen"},
    {"Saved as draft", "Als Entwurf gespeichert"},
    {"Draft deleted", "Entwurf gelöscht"},
    {"Drafts full.\nSend or discard.",
     "Entwürfe voll.\nSenden oder verwerfen."},
    {"Phone unreachable.\nSaved as draft.",
     "Handy nicht erreichbar.\nAls Entwurf gespeichert."},
    {"Nothing to send", "Nichts zu senden"},
    {"Sending...", "Wird gesendet..."},
    {"Dictation Failed", "Diktat fehlgeschlagen"},
    {"Memo too long", "Memo zu lang"},
    // lists
    {"Load more", "Mehr laden"},
    {"No drafts", "Keine Entwürfe"},
    {"No matches", "Keine Treffer"},
    {"No memos", "Keine Memos"},
    {"No pinned memos", "Keine angepinnten Memos"},
    {"No tags", "Keine Tags"},
    {"No tasks", "Keine Aufgaben"},
    {"No comments yet", "Noch keine Kommentare"},
    // connection
    {"No response from phone", "Keine Antwort vom Handy"},
    {"Phone not connected", "Handy nicht verbunden"},
    {"Request failed", "Anfrage fehlgeschlagen"},
    // app glance
    {"No open todos", "Keine offenen Aufgaben"},
    {"1 open todo", "1 offene Aufgabe"},
    {"%d open todos", "%d offene Aufgaben"},
};

static Language s_setting = LanguageAuto;
static bool s_german;

static void update(void) {
  if (s_setting == LanguageAuto) {
    s_german = strncmp(i18n_get_system_locale(), "de", 2) == 0;
  } else {
    s_german = s_setting == LanguageGerman;
  }
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
  if (!s_german) {
    return english;
  }
  for (size_t i = 0; i < ARRAY_LENGTH(GERMAN); i++) {
    if (strcmp(GERMAN[i].english, english) == 0) {
      return GERMAN[i].german;
    }
  }
  return english;
}
