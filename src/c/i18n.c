#include "i18n.h"

#define PERSIST_KEY_LANGUAGE 51

// Languages with a column in TRANSLATIONS, in the order of the columns
#define TRANSLATED_LANGUAGES 4

typedef struct {
  const char *english;
  // German, French, Spanish, Italian
  const char *texts[TRANSLATED_LANGUAGES];
} Translation;

static const Translation TRANSLATIONS[] = {
    // main menu
    {"New", {"Neu", "Nouveau", "Nuevo", "Nuovo"}},
    {"Dictate a memo",
     {"Memo diktieren", "Dicter un mémo", "Dictar un memo", "Detta un memo"}},
    {"Recent", {"Neueste", "Récents", "Recientes", "Recenti"}},
    {"Latest memos",
     {"Letzte Memos", "Derniers mémos", "Últimos memos", "Ultimi memo"}},
    {"Pinned", {"Angepinnt", "Épinglés", "Fijados", "Fissati"}},
    {"Pinned memos",
     {"Angepinnte Memos", "Mémos épinglés", "Memos fijados", "Memo fissati"}},
    {"Tags", {"Tags", "Tags", "Etiquetas", "Tag"}},
    {"Memos by tag",
     {"Memos nach Tag", "Mémos par tag", "Memos por etiqueta", "Memo per tag"}},
    {"Search", {"Suche", "Recherche", "Buscar", "Cerca"}},
    {"Dictate words to find",
     {"Suchbegriff diktieren", "Dicter les mots à chercher",
      "Dictar palabras a buscar", "Detta le parole da cercare"}},
    {"Drafts", {"Entwürfe", "Brouillons", "Borradores", "Bozze"}},
    {"None saved",
     {"Keine Entwürfe", "Aucun brouillon", "Sin borradores", "Nessuna bozza"}},
    {"%d saved",
     {"%d gespeichert", "%d brouillons", "%d borradores", "%d bozze"}},
    // title bars
    {"Draft", {"Entwurf", "Brouillon", "Borrador", "Bozza"}},
    {"Comment", {"Kommentar", "Commentaire", "Comentario", "Commento"}},
    {"Append", {"Anhängen", "Ajouter", "Añadir", "Aggiungi"}},
    {"Memo", {"Memo", "Mémo", "Memo", "Memo"}},
    {"Comments", {"Kommentare", "Commentaires", "Comentarios", "Commenti"}},
    // actions
    {"Send", {"Senden", "Envoyer", "Enviar", "Invia"}},
    {"Dictate more",
     {"Weiter diktieren", "Dicter la suite", "Seguir dictando",
      "Continua a dettare"}},
    {"Discard", {"Verwerfen", "Abandonner", "Descartar", "Scarta"}},
    {"Yes, discard",
     {"Ja, verwerfen", "Oui, abandonner", "Sí, descartar", "Sì, scarta"}},
    {"Open", {"Öffnen", "Ouvrir", "Abrir", "Apri"}},
    {"Tasks", {"Aufgaben", "Tâches", "Tareas", "Attività"}},
    {"New comment",
     {"Neuer Kommentar", "Nouveau commentaire", "Nuevo comentario",
      "Nuovo commento"}},
    {"Pin / Unpin",
     {"Anpinnen / Lösen", "Épingler / Désépingler", "Fijar / Soltar",
      "Fissa / Sblocca"}},
    {"Archive", {"Archivieren", "Archiver", "Archivar", "Archivia"}},
    {"Delete", {"Löschen", "Supprimer", "Eliminar", "Elimina"}},
    {"Yes, delete",
     {"Ja, löschen", "Oui, supprimer", "Sí, eliminar", "Sì, elimina"}},
    {"Cancel", {"Abbrechen", "Annuler", "Cancelar", "Annulla"}},
    // results
    {"Archived", {"Archiviert", "Archivé", "Archivado", "Archiviato"}},
    {"Deleted", {"Gelöscht", "Supprimé", "Eliminado", "Eliminato"}},
    {"Sent", {"Gesendet", "Envoyé", "Enviado", "Inviato"}},
    {"Comment added",
     {"Kommentar hinzugefügt", "Commentaire ajouté", "Comentario añadido",
      "Commento aggiunto"}},
    {"Appended", {"Angehängt", "Ajouté", "Añadido", "Aggiunto"}},
    {"Comment discarded",
     {"Kommentar verworfen", "Commentaire abandonné", "Comentario descartado",
      "Commento scartato"}},
    {"Append discarded",
     {"Anhängen verworfen", "Ajout abandonné", "Añadido descartado",
      "Aggiunta scartata"}},
    {"Saved as draft",
     {"Als Entwurf gespeichert", "Enregistré en brouillon",
      "Guardado como borrador", "Salvato come bozza"}},
    {"Draft deleted",
     {"Entwurf gelöscht", "Brouillon supprimé", "Borrador eliminado",
      "Bozza eliminata"}},
    {"Drafts full.\nSend or discard.",
     {"Entwürfe voll.\nSenden oder verwerfen.",
      "Brouillons pleins.\nEnvoyez ou abandonnez.",
      "Borradores llenos.\nEnvía o descarta.",
      "Bozze piene.\nInvia o scarta."}},
    {"Phone unreachable.\nSaved as draft.",
     {"Handy nicht erreichbar.\nAls Entwurf gespeichert.",
      "Téléphone injoignable.\nEnregistré en brouillon.",
      "Teléfono inaccesible.\nGuardado como borrador.",
      "Telefono non raggiungibile.\nSalvato come bozza."}},
    {"Nothing to send",
     {"Nichts zu senden", "Rien à envoyer", "Nada que enviar",
      "Niente da inviare"}},
    {"Sending...", {"Wird gesendet...", "Envoi...", "Enviando...", "Invio..."}},
    {"Dictation Failed",
     {"Diktat fehlgeschlagen", "Échec de la dictée", "Error de dictado",
      "Dettatura non riuscita"}},
    {"Memo too long",
     {"Memo zu lang", "Mémo trop long", "Memo demasiado largo",
      "Memo troppo lungo"}},
    // lists
    {"Load more", {"Mehr laden", "Charger plus", "Cargar más", "Carica altri"}},
    {"No drafts",
     {"Keine Entwürfe", "Aucun brouillon", "Sin borradores", "Nessuna bozza"}},
    {"No matches",
     {"Keine Treffer", "Aucun résultat", "Sin resultados", "Nessun risultato"}},
    {"No memos", {"Keine Memos", "Aucun mémo", "Sin memos", "Nessun memo"}},
    {"No pinned memos",
     {"Keine angepinnten Memos", "Aucun mémo épinglé", "Sin memos fijados",
      "Nessun memo fissato"}},
    {"No tags", {"Keine Tags", "Aucun tag", "Sin etiquetas", "Nessun tag"}},
    {"No tasks",
     {"Keine Aufgaben", "Aucune tâche", "Sin tareas", "Nessuna attività"}},
    {"No comments yet",
     {"Noch keine Kommentare", "Pas encore de commentaires",
      "Aún no hay comentarios", "Ancora nessun commento"}},
    // connection
    {"No response from phone",
     {"Keine Antwort vom Handy", "Pas de réponse du téléphone",
      "Sin respuesta del teléfono", "Nessuna risposta dal telefono"}},
    {"Phone not connected",
     {"Handy nicht verbunden", "Téléphone non connecté",
      "Teléfono no conectado", "Telefono non connesso"}},
    {"Request failed",
     {"Anfrage fehlgeschlagen", "Échec de la requête", "Error en la solicitud",
      "Richiesta non riuscita"}},
    {"Offline: saved on watch",
     {"Offline: auf der Uhr gespeichert",
      "Hors ligne : enregistré sur la montre",
      "Sin conexión: guardado en el reloj", "Offline: salvato sull'orologio"}},
    // app glance
    {"No open todos",
     {"Keine offenen Aufgaben", "Aucune tâche ouverte", "Sin tareas pendientes",
      "Nessuna attività aperta"}},
    {"1 open todo",
     {"1 offene Aufgabe", "1 tâche ouverte", "1 tarea pendiente",
      "1 attività aperta"}},
    {"%d open todos",
     {"%d offene Aufgaben", "%d tâches ouvertes", "%d tareas pendientes",
      "%d attività aperte"}},
};

static Language s_setting = LanguageAuto;
// column in TRANSLATIONS, or -1 for English
static int s_column = -1;

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

static void update(void) {
  Language language = s_setting == LanguageAuto ? system_language() : s_setting;
  if (language >= LanguageGerman &&
      language < LanguageGerman + TRANSLATED_LANGUAGES) {
    s_column = language - LanguageGerman;
  } else {
    s_column = -1;
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
  if (s_column < 0) {
    return english;
  }
  for (size_t i = 0; i < ARRAY_LENGTH(TRANSLATIONS); i++) {
    if (strcmp(TRANSLATIONS[i].english, english) == 0) {
      return TRANSLATIONS[i].texts[s_column];
    }
  }
  return english;
}
