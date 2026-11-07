// Translations of the texts the phone sends to the watch and of the
// settings page. Texts are written in English in the code and translated
// only when they leave the phone, so the logic can compare English texts.

// Language setting values, sent to the watch as these codes
// (must match Language in i18n.h)
var LANGUAGE_CODES = {auto: 0, en: 1, de: 2, fr: 3, es: 4, it: 5};

var TRANSLATIONS = {
  de: {
    'Not configured': 'Nicht eingerichtet',
    'Network error': 'Netzwerkfehler',
    'Request timed out': 'Zeitüberschreitung',
    'Invalid response': 'Ungültige Antwort',
    'Memo is empty': 'Memo ist leer',
    'Comment is empty': 'Kommentar ist leer',
    'Nothing to append': 'Nichts zum Anhängen',
    'Nothing to search': 'Kein Suchbegriff',
    'Memo not found': 'Memo nicht gefunden',
    'Memo changed, reload tasks': 'Memo geändert, Aufgaben neu laden',
    'Phone error': 'Fehler auf dem Handy',
    'Offline: cached memos': 'Offline: gespeicherte Memos',
    'Offline:\nMemo queued': 'Offline:\nMemo vorgemerkt',
    'Queued memo sent': 'Vorgemerktes Memo gesendet',
    'Connected to Memos': 'Mit Memos verbunden',
    'API not found, check URL': 'API nicht gefunden, URL prüfen',
    'Server not reachable': 'Server nicht erreichbar',
    'Pinned': 'Angepinnt',
    'Unpinned': 'Gelöst',
    '(empty)': '(leer)',
    '1 memo': '1 Memo'
  },
  fr: {
    'Not configured': 'Non configuré',
    'Network error': 'Erreur réseau',
    'Request timed out': 'Délai dépassé',
    'Invalid response': 'Réponse invalide',
    'Memo is empty': 'Le mémo est vide',
    'Comment is empty': 'Le commentaire est vide',
    'Nothing to append': 'Rien à ajouter',
    'Nothing to search': 'Rien à chercher',
    'Memo not found': 'Mémo introuvable',
    'Memo changed, reload tasks': 'Mémo modifié, rechargez les tâches',
    'Phone error': 'Erreur du téléphone',
    'Offline: cached memos': 'Hors ligne\u00a0: mémos en cache',
    'Offline:\nMemo queued': 'Hors ligne\u00a0:\nMémo en attente',
    'Queued memo sent': 'Mémo en attente envoyé',
    'Connected to Memos': 'Connecté à Memos',
    'API not found, check URL': 'API introuvable, vérifiez l\'URL',
    'Server not reachable': 'Serveur injoignable',
    'Pinned': 'Épinglé',
    'Unpinned': 'Désépinglé',
    '(empty)': '(vide)',
    '1 memo': '1 mémo'
  },
  es: {
    'Not configured': 'Sin configurar',
    'Network error': 'Error de red',
    'Request timed out': 'Tiempo de espera agotado',
    'Invalid response': 'Respuesta no válida',
    'Memo is empty': 'El memo está vacío',
    'Comment is empty': 'El comentario está vacío',
    'Nothing to append': 'Nada que añadir',
    'Nothing to search': 'Nada que buscar',
    'Memo not found': 'Memo no encontrado',
    'Memo changed, reload tasks': 'Memo modificado, recarga las tareas',
    'Phone error': 'Error del teléfono',
    'Offline: cached memos': 'Sin conexión: memos guardados',
    'Offline:\nMemo queued': 'Sin conexión:\nMemo en cola',
    'Queued memo sent': 'Memo en cola enviado',
    'Connected to Memos': 'Conectado a Memos',
    'API not found, check URL': 'API no encontrada, revisa la URL',
    'Server not reachable': 'Servidor inaccesible',
    'Pinned': 'Fijado',
    'Unpinned': 'Soltado',
    '(empty)': '(vacío)',
    '1 memo': '1 memo'
  },
  it: {
    'Not configured': 'Non configurato',
    'Network error': 'Errore di rete',
    'Request timed out': 'Tempo scaduto',
    'Invalid response': 'Risposta non valida',
    'Memo is empty': 'Il memo è vuoto',
    'Comment is empty': 'Il commento è vuoto',
    'Nothing to append': 'Niente da aggiungere',
    'Nothing to search': 'Niente da cercare',
    'Memo not found': 'Memo non trovato',
    'Memo changed, reload tasks': 'Memo modificato, ricarica le attività',
    'Phone error': 'Errore del telefono',
    'Offline: cached memos': 'Offline: memo salvati',
    'Offline:\nMemo queued': 'Offline:\nMemo in coda',
    'Queued memo sent': 'Memo in coda inviato',
    'Connected to Memos': 'Connesso a Memos',
    'API not found, check URL': 'API non trovata, controlla l\'URL',
    'Server not reachable': 'Server non raggiungibile',
    'Pinned': 'Fissato',
    'Unpinned': 'Sbloccato',
    '(empty)': '(vuoto)',
    '1 memo': '1 memo'
  }
};

// Texts with a variable part; $1 in a translation is the translated match
var PATTERNS = [
  [
    /^Server error (\d+)$/, {
      de: 'Serverfehler $1',
      fr: 'Erreur du serveur $1',
      es: 'Error del servidor $1',
      it: 'Errore del server $1'
    }
  ],
  [
    /^Token rejected \((\d+)\)$/, {
      de: 'Token abgelehnt ($1)',
      fr: 'Jeton refusé ($1)',
      es: 'Token rechazado ($1)',
      it: 'Token rifiutato ($1)'
    }
  ],
  [
    /^(\d+) memos$/,
    {de: '$1 Memos', fr: '$1 mémos', es: '$1 memos', it: '$1 memo'}
  ],
  [
    /^(\d+) queued memos sent$/, {
      de: '$1 vorgemerkte Memos gesendet',
      fr: '$1 mémos en attente envoyés',
      es: '$1 memos en cola enviados',
      it: '$1 memo in coda inviati'
    }
  ],
  [
    /^Queued memo rejected: (.*)$/, {
      de: 'Vorgemerktes Memo abgelehnt: $1',
      fr: 'Mémo en attente refusé\u00a0: $1',
      es: 'Memo en cola rechazado: $1',
      it: 'Memo in coda rifiutato: $1'
    }
  ]
];

// Short month names, and how a day and month are put together
var DATES = {
  en: {
    months: [
      'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct',
      'Nov', 'Dec'
    ],
    dayMonth: function(day, month) {
      return month + ' ' + day;
    }
  },
  de: {
    months: [
      'Jan', 'Feb', 'Mär', 'Apr', 'Mai', 'Jun', 'Jul', 'Aug', 'Sep', 'Okt',
      'Nov', 'Dez'
    ],
    dayMonth: function(day, month) {
      return day + '. ' + month;
    }
  },
  fr: {
    months: [
      'janv.', 'févr.', 'mars', 'avr.', 'mai', 'juin', 'juil.', 'août', 'sept.',
      'oct.', 'nov.', 'déc.'
    ],
    dayMonth: function(day, month) {
      return day + ' ' + month;
    }
  },
  es: {
    months: [
      'ene', 'feb', 'mar', 'abr', 'may', 'jun', 'jul', 'ago', 'sept', 'oct',
      'nov', 'dic'
    ],
    dayMonth: function(day, month) {
      return day + ' ' + month;
    }
  },
  it: {
    months: [
      'gen', 'feb', 'mar', 'apr', 'mag', 'giu', 'lug', 'ago', 'set', 'ott',
      'nov', 'dic'
    ],
    dayMonth: function(day, month) {
      return day + ' ' + month;
    }
  }
};

function translate(text, language) {
  var table = TRANSLATIONS[language];
  if (!table) return text;
  if (Object.prototype.hasOwnProperty.call(table, text)) return table[text];
  for (var i = 0; i < PATTERNS.length; i++) {
    var m = text.match(PATTERNS[i][0]);
    if (m) {
      return PATTERNS[i][1][language].replace(/\$(\d)/g, function(ref, n) {
        return translate(m[n], language);
      });
    }
  }
  return text;
}

function settingValue(setting) {
  return LANGUAGE_CODES.hasOwnProperty(setting) ? setting : 'auto';
}

// The language of the watch, or of the phone if the watch is unknown
function systemLanguage() {
  var locale = '';
  try {
    var info = Pebble.getActiveWatchInfo && Pebble.getActiveWatchInfo();
    locale = (info && info.language) || '';
  } catch (e) {
    // no watch connected
  }
  if (!locale && typeof navigator !== 'undefined')
    locale = navigator.language || '';
  var code = locale.slice(0, 2).toLowerCase();
  return TRANSLATIONS.hasOwnProperty(code) ? code : 'en';
}

// "en", "de", "fr", "es" or "it" for a language setting ("auto" or one of
// these)
function effectiveLanguage(setting) {
  setting = settingValue(setting);
  return setting === 'auto' ? systemLanguage() : setting;
}

// "Oct 18, 14:32", "18. Okt, 14:32", "18 oct., 14:32", ...
function formatDate(date, language) {
  var time = (date.getHours() < 10 ? '0' : '') + date.getHours() + ':' +
      (date.getMinutes() < 10 ? '0' : '') + date.getMinutes();
  var dates = DATES[language] || DATES.en;
  return dates.dayMonth(date.getDate(), dates.months[date.getMonth()]) + ', ' +
      time;
}

// Texts of the settings page
var PAGE = {
  en: {
    tagline: 'Your self-hosted Memos server on your wrist.',
    connection: 'Connection',
    url: 'Memos Server URL',
    urlHint: 'Example: https://memos.example.com',
    token: 'Access Token',
    tokenHint:
        'Create one in Memos under Settings, Access Tokens. The watch checks it after saving.',
    tokenPlaceholder: 'Your access token',
    test: 'Check server',
    memos: 'Memos',
    tags: 'Default Tags',
    tagsHint: 'Added to every new memo, separated by spaces.',
    todoTag: 'To-do Tag',
    todoTagHint:
        'The app launcher shows the open tasks of all memos with this tag.',
    keywords: 'Spoken Keywords',
    keywordsHint:
        'Spoken words and the markdown they become; <strong>\\n</strong> is a line break. The ones of your language are filled in; until you change them, those of the other languages work, too.',
    keywordsReset: 'Reset',
    keywordWord: 'Word, e.g. quote',
    keywordMarkdown: 'Markdown, e.g. >',
    keywordAdd: 'Add keyword',
    keywordRemove: 'Remove keyword',
    watch: 'Watch',
    vibrate: 'Vibrate on success and errors',
    language: 'Language',
    languageAuto: 'Automatic (watch language)',
    save: 'Save',
    // used by clay-custom.js
    testMissing: 'Enter the server URL first.',
    testRunning: 'Checking...',
    testOk: 'Server reachable. The watch checks the token after saving.',
    testUnreachable:
        'Server not reachable from this page. Check the URL. A server in your local network can only be checked by the watch, after saving.'
  },
  de: {
    tagline: 'Dein selbst gehosteter Memos-Server am Handgelenk.',
    connection: 'Verbindung',
    url: 'Memos-Server-URL',
    urlHint: 'Beispiel: https://memos.example.com',
    token: 'Zugangstoken',
    tokenHint:
        'Erstelle eins in Memos unter Einstellungen, Zugangstoken. Die Uhr prüft es nach dem Speichern.',
    tokenPlaceholder: 'Dein Zugangstoken',
    test: 'Server prüfen',
    memos: 'Memos',
    tags: 'Standard-Tags',
    tagsHint: 'Werden jedem neuen Memo angehängt, durch Leerzeichen getrennt.',
    todoTag: 'Aufgaben-Tag',
    todoTagHint:
        'Der App-Launcher zeigt die offenen Aufgaben aller Memos mit diesem Tag.',
    keywords: 'Gesprochene Schlüsselwörter',
    keywordsHint:
        'Gesprochene Wörter und das Markdown, zu dem sie werden; <strong>\\n</strong> ist ein Zeilenumbruch. Die deiner Sprache sind eingetragen; bis du sie änderst, wirken auch die der anderen Sprachen.',
    keywordsReset: 'Zurücksetzen',
    keywordWord: 'Wort, z. B. Zitat',
    keywordMarkdown: 'Markdown, z. B. >',
    keywordAdd: 'Schlüsselwort hinzufügen',
    keywordRemove: 'Schlüsselwort entfernen',
    watch: 'Uhr',
    vibrate: 'Bei Erfolg und Fehlern vibrieren',
    language: 'Sprache',
    languageAuto: 'Automatisch (Sprache der Uhr)',
    save: 'Speichern',
    testMissing: 'Bitte zuerst die Server-URL eingeben.',
    testRunning: 'Wird geprüft...',
    testOk: 'Server erreichbar. Das Token prüft die Uhr nach dem Speichern.',
    testUnreachable:
        'Server von dieser Seite aus nicht erreichbar. Prüfe die URL. Einen Server im Heimnetz kann nur die Uhr prüfen, nach dem Speichern.'
  },
  fr: {
    tagline: 'Votre serveur Memos auto-hébergé à votre poignet.',
    connection: 'Connexion',
    url: 'URL du serveur Memos',
    urlHint: 'Exemple\u00a0: https://memos.example.com',
    token: 'Jeton d\'accès',
    tokenHint:
        'Créez-en un dans Memos sous Paramètres, Jetons d\'accès. La montre le vérifie après l\'enregistrement.',
    tokenPlaceholder: 'Votre jeton d\'accès',
    test: 'Vérifier le serveur',
    memos: 'Mémos',
    tags: 'Tags par défaut',
    tagsHint: 'Ajoutés à chaque nouveau mémo, séparés par des espaces.',
    todoTag: 'Tag des tâches',
    todoTagHint: 'Le lanceur d\'applications affiche les tâches ouvertes ' +
        'de tous les mémos portant ce tag.',
    keywords: 'Mots-clés vocaux',
    keywordsHint:
        'Mots prononcés et le markdown qu\'ils deviennent ; <strong>\\n</strong> est un saut de ligne. Ceux de votre langue sont préremplis ; tant que vous ne les modifiez pas, ceux des autres langues fonctionnent aussi.',
    keywordsReset: 'Réinitialiser',
    keywordWord: 'Mot, p. ex. citation',
    keywordMarkdown: 'Markdown, p. ex. >',
    keywordAdd: 'Ajouter un mot-clé',
    keywordRemove: 'Supprimer le mot-clé',
    watch: 'Montre',
    vibrate: 'Vibrer en cas de succès ou d\'erreur',
    language: 'Langue',
    languageAuto: 'Automatique (langue de la montre)',
    save: 'Enregistrer',
    testMissing: 'Saisissez d\'abord l\'URL du serveur.',
    testRunning: 'Vérification...',
    testOk:
        'Serveur joignable. La montre vérifie le jeton après l\'enregistrement.',
    testUnreachable:
        'Serveur injoignable depuis cette page. Vérifiez l\'URL. Un serveur de votre réseau local ne peut être vérifié que par la montre, après l\'enregistrement.'
  },
  es: {
    tagline: 'Tu servidor Memos autoalojado en tu muñeca.',
    connection: 'Conexión',
    url: 'URL del servidor Memos',
    urlHint: 'Ejemplo: https://memos.example.com',
    token: 'Token de acceso',
    tokenHint:
        'Crea uno en Memos en Ajustes, Tokens de acceso. El reloj lo comprueba al guardar.',
    tokenPlaceholder: 'Tu token de acceso',
    test: 'Comprobar servidor',
    memos: 'Memos',
    tags: 'Etiquetas predeterminadas',
    tagsHint: 'Se añaden a cada memo nuevo, separadas por espacios.',
    todoTag: 'Etiqueta de tareas',
    todoTagHint: 'El lanzador de apps muestra las tareas pendientes de ' +
        'todos los memos con esta etiqueta.',
    keywords: 'Palabras clave habladas',
    keywordsHint:
        'Palabras habladas y el markdown en que se convierten; <strong>\\n</strong> es un salto de línea. Las de tu idioma ya están puestas; mientras no las cambies, también funcionan las de los otros idiomas.',
    keywordsReset: 'Restablecer',
    keywordWord: 'Palabra, p. ej. cita',
    keywordMarkdown: 'Markdown, p. ej. >',
    keywordAdd: 'Añadir palabra clave',
    keywordRemove: 'Quitar palabra clave',
    watch: 'Reloj',
    vibrate: 'Vibrar en aciertos y errores',
    language: 'Idioma',
    languageAuto: 'Automático (idioma del reloj)',
    save: 'Guardar',
    testMissing: 'Introduce primero la URL del servidor.',
    testRunning: 'Comprobando...',
    testOk: 'Servidor accesible. El reloj comprueba el token al guardar.',
    testUnreachable:
        'Servidor no accesible desde esta página. Comprueba la URL. Un servidor de tu red local solo lo puede comprobar el reloj, al guardar.'
  },
  it: {
    tagline: 'Il tuo server Memos self-hosted al polso.',
    connection: 'Connessione',
    url: 'URL del server Memos',
    urlHint: 'Esempio: https://memos.example.com',
    token: 'Token di accesso',
    tokenHint:
        'Creane uno in Memos in Impostazioni, Token di accesso. L\'orologio lo verifica dopo il salvataggio.',
    tokenPlaceholder: 'Il tuo token di accesso',
    test: 'Verifica server',
    memos: 'Memo',
    tags: 'Tag predefiniti',
    tagsHint: 'Aggiunti a ogni nuovo memo, separati da spazi.',
    todoTag: 'Tag delle attività',
    todoTagHint: 'Il launcher mostra le attività aperte di tutti i memo ' +
        'con questo tag.',
    keywords: 'Parole chiave vocali',
    keywordsHint:
        'Parole pronunciate e il markdown in cui si trasformano; <strong>\\n</strong> è un a capo. Quelle della tua lingua sono già inserite; finché non le modifichi, funzionano anche quelle delle altre lingue.',
    keywordsReset: 'Ripristina',
    keywordWord: 'Parola, ad es. citazione',
    keywordMarkdown: 'Markdown, ad es. >',
    keywordAdd: 'Aggiungi parola chiave',
    keywordRemove: 'Rimuovi parola chiave',
    watch: 'Orologio',
    vibrate: 'Vibra in caso di successo ed errori',
    language: 'Lingua',
    languageAuto: 'Automatica (lingua dell\'orologio)',
    save: 'Salva',
    testMissing: 'Inserisci prima l\'URL del server.',
    testRunning: 'Verifica in corso...',
    testOk:
        'Server raggiungibile. L\'orologio verifica il token dopo il salvataggio.',
    testUnreachable:
        'Server non raggiungibile da questa pagina. Controlla l\'URL. Un server nella rete locale può essere verificato solo dall\'orologio, dopo il salvataggio.'
  }
};

function pageStrings(language) {
  return PAGE[language] || PAGE.en;
}

module.exports = {
  LANGUAGE_CODES : LANGUAGE_CODES,
  settingValue : settingValue,
  effectiveLanguage : effectiveLanguage,
  translate : translate,
  formatDate : formatDate,
  pageStrings : pageStrings
};
