// Translations of the texts the phone sends to the watch and of the
// settings page. Texts are written in English in the code and translated
// only when they leave the phone, so the logic can compare English texts.

// Language setting values, sent to the watch as these codes
// (must match Language in i18n.h)
var LANGUAGE_CODES = {auto: 0, en: 1, de: 2};

var GERMAN = {
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
  'Offline: memo queued': 'Offline: Memo vorgemerkt',
  'Queued memo sent': 'Vorgemerktes Memo gesendet',
  'Connected to Memos': 'Mit Memos verbunden',
  'API not found, check URL': 'API nicht gefunden, URL prüfen',
  'Server not reachable': 'Server nicht erreichbar',
  'Pinned': 'Angepinnt',
  'Unpinned': 'Gelöst',
  'No comments yet': 'Noch keine Kommentare',
  '(empty)': '(leer)',
  '1 memo': '1 Memo'
};

// Texts with a variable part
var GERMAN_PATTERNS = [
  [
    /^Server error (\d+)$/,
    function(m) {
      return 'Serverfehler ' + m[1];
    }
  ],
  [
    /^Token rejected \((\d+)\)$/,
    function(m) {
      return 'Token abgelehnt (' + m[1] + ')';
    }
  ],
  [
    /^(\d+) memos$/,
    function(m) {
      return m[1] + ' Memos';
    }
  ],
  [
    /^(\d+) queued memos sent$/,
    function(m) {
      return m[1] + ' vorgemerkte Memos gesendet';
    }
  ],
  [
    /^Queued memo rejected: (.*)$/,
    function(m) {
      return 'Vorgemerktes Memo abgelehnt: ' + translateGerman(m[1]);
    }
  ]
];

var GERMAN_MONTHS = [
  'Jan', 'Feb', 'Mär', 'Apr', 'Mai', 'Jun', 'Jul', 'Aug', 'Sep', 'Okt', 'Nov',
  'Dez'
];
var ENGLISH_MONTHS = [
  'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov',
  'Dec'
];

function translateGerman(text) {
  if (Object.prototype.hasOwnProperty.call(GERMAN, text)) return GERMAN[text];
  for (var i = 0; i < GERMAN_PATTERNS.length; i++) {
    var m = text.match(GERMAN_PATTERNS[i][0]);
    if (m) return GERMAN_PATTERNS[i][1](m);
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
  return /^de/i.test(locale) ? 'de' : 'en';
}

// "en" or "de" for a language setting ("auto", "en" or "de")
function effectiveLanguage(setting) {
  setting = settingValue(setting);
  return setting === 'auto' ? systemLanguage() : setting;
}

function translate(text, language) {
  return language === 'de' ? translateGerman(text) : text;
}

// "Oct 18, 14:32" or "18. Okt, 14:32"
function formatDate(date, language) {
  var time = (date.getHours() < 10 ? '0' : '') + date.getHours() + ':' +
      (date.getMinutes() < 10 ? '0' : '') + date.getMinutes();
  if (language === 'de') {
    return date.getDate() + '. ' + GERMAN_MONTHS[date.getMonth()] + ', ' + time;
  }
  return ENGLISH_MONTHS[date.getMonth()] + ' ' + date.getDate() + ', ' + time;
}

// Texts of the settings page
var PAGE = {
  en: {
    tagline: 'Your self-hosted Memos server on your wrist.',
    connection: 'Connection',
    url: 'Memos Server URL',
    urlHint: 'Example: https://memos.example.com',
    token: 'Access Token',
    tokenHint: 'Create one in Memos under Settings, Access Tokens.',
    tokenPlaceholder: 'Your access token',
    test: 'Test connection',
    memos: 'Memos',
    tags: 'Default Tags',
    tagsHint: 'Added to every new memo, separated by spaces.',
    todoTag: 'To-do Tag',
    todoTagHint:
        'The app launcher shows the open tasks of all memos with this tag.',
    watch: 'Watch',
    vibrate: 'Vibrate on success and errors',
    language: 'Language',
    languageAuto: 'Automatic (watch language)',
    save: 'Save',
    // used by clay-custom.js
    testMissing: 'Enter the server URL and token first.',
    testRunning: 'Testing...',
    testOk: 'Connected. URL and token work.',
    testRejected: 'The server rejected the token (%s).',
    testNotFound: 'No Memos API found at this URL (404).',
    testServerError: 'Server error %s.',
    testUnavailable: 'Could not test from this page. Save, and the watch ' +
        'will show whether the connection works.'
  },
  de: {
    tagline: 'Dein selbst gehosteter Memos-Server am Handgelenk.',
    connection: 'Verbindung',
    url: 'Memos-Server-URL',
    urlHint: 'Beispiel: https://memos.example.com',
    token: 'Zugangstoken',
    tokenHint: 'Erstelle eins in Memos unter Einstellungen, Zugangstoken.',
    tokenPlaceholder: 'Dein Zugangstoken',
    test: 'Verbindung testen',
    memos: 'Memos',
    tags: 'Standard-Tags',
    tagsHint: 'Werden jedem neuen Memo angehängt, durch Leerzeichen getrennt.',
    todoTag: 'Aufgaben-Tag',
    todoTagHint:
        'Der App-Launcher zeigt die offenen Aufgaben aller Memos mit diesem Tag.',
    watch: 'Uhr',
    vibrate: 'Bei Erfolg und Fehlern vibrieren',
    language: 'Sprache',
    languageAuto: 'Automatisch (Sprache der Uhr)',
    save: 'Speichern',
    testMissing: 'Bitte zuerst Server-URL und Token eingeben.',
    testRunning: 'Wird getestet...',
    testOk: 'Verbunden. URL und Token funktionieren.',
    testRejected: 'Der Server hat das Token abgelehnt (%s).',
    testNotFound: 'Keine Memos-API unter dieser URL gefunden (404).',
    testServerError: 'Serverfehler %s.',
    testUnavailable: 'Test von dieser Seite nicht möglich. Speichere, dann ' +
        'zeigt die Uhr, ob die Verbindung funktioniert.'
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
