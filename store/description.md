# Store listing

## English

**Name:** Ostrakon

**Short description:** Dictate, browse and organize your Memos notes from your wrist.

**Description:**

Ostrakon brings your self-hosted Memos server to your Pebble. Dictate a note in seconds, read your recent memos with their formatting, tick off tasks and add comments, without taking out your phone.

Write
• New memos by voice; spoken keywords turn into checkboxes, headings, line breaks and tags
• Append to an existing memo or comment on it
• Drafts are kept on the watch and work without a connection
• Without internet, new memos are queued and sent automatically later

Read
• Recent, pinned and tagged memos, with "Load more"
• Search by voice
• Memos are shown with headings, checkboxes, lists and quotes
• Comments are listed with their author and time
• Recently loaded memos are cached on the phone and also open offline

Organize
• Tick off the tasks of a memo; changes are saved to your server
• Pin, archive and delete memos; archive and delete comments
• The launcher shows how many tasks are open in your to-do memos

Setup: open the app settings on your phone, enter your Memos server URL and an access token, and test the connection right there.

Privacy: Ostrakon only talks to the Memos server you configure. Your URL and token are stored on your phone. There is no tracking and no third-party service.

Works on all current Pebble watches, including Pebble Time 2, Pebble 2 Duo and Pebble Round 2.

Ostrakon is an independent app for Memos (usememos.com) and is not affiliated with the Memos project. It started as a fork of pebble-memos by Erik Morris and has since been rewritten; many thanks to him for the idea.

**Category:** Tools & Utilities

## Deutsch

**Name:** Ostrakon

**Kurzbeschreibung:** Notizen in Memos diktieren, lesen und organisieren, direkt am Handgelenk.

**Beschreibung:**

Ostrakon bringt deinen selbst gehosteten Memos-Server auf die Pebble. Diktiere eine Notiz in Sekunden, lies deine letzten Memos mit Formatierung, hake Aufgaben ab und schreibe Kommentare, ohne das Handy herauszuholen.

Schreiben
• Neue Memos per Sprache; gesprochene Schlüsselwörter werden zu Checkboxen, Überschriften, Zeilenumbrüchen und Tags
• Text an ein Memo anhängen oder es kommentieren
• Entwürfe bleiben auf der Uhr und funktionieren auch ohne Verbindung
• Ohne Internet werden neue Memos vorgemerkt und später automatisch gesendet

Lesen
• Neueste, angepinnte und getaggte Memos, mit „Mehr laden“
• Suche per Sprache
• Memos mit Überschriften, Checkboxen, Listen und Zitaten
• Kommentare mit Autor und Uhrzeit
• Zuletzt geladene Memos werden auf dem Handy zwischengespeichert und öffnen auch offline

Organisieren
• Aufgaben eines Memos abhaken; Änderungen werden auf deinem Server gespeichert
• Memos anpinnen, archivieren und löschen, Kommentare archivieren und löschen
• Der Launcher zeigt, wie viele Aufgaben in deinen To-do-Memos offen sind

Einrichtung: Öffne die App-Einstellungen auf dem Handy, gib die URL deines Memos-Servers und einen Zugriffstoken ein und teste die Verbindung direkt dort.

Datenschutz: Ostrakon spricht nur mit dem Memos-Server, den du einträgst. URL und Token werden auf deinem Handy gespeichert. Es gibt kein Tracking und keine Drittanbieter.

Läuft auf allen aktuellen Pebble-Uhren, auch auf Pebble Time 2, Pebble 2 Duo und Pebble Round 2.

Ostrakon ist eine unabhängige App für Memos (usememos.com) und gehört nicht zum Memos-Projekt. Sie entstand als Fork von pebble-memos von Erik Morris und wurde seitdem neu geschrieben; vielen Dank an ihn für die Idee.

**Kategorie:** Werkzeuge & Hilfsmittel

## Graphics

* `icon-144.png`: large store icon
* `icon-48.png`: small store icon
* `screenshots/`: seven screenshots per platform, taken in the emulator with demo data: `basalt-*` (rectangular, 144×168, also for diorite and flint), `chalk-*` (round, 180×180, also for gabbro) and `emery-*` (large, 200×228). In this order:
  1. `recent`: the latest memos
  2. `memo`: a memo with heading, tasks and tags
  3. `tasks`: ticking off the tasks of a memo
  4. `comments`: the comments of a memo with author and time
  5. `comment`: a single comment
  6. `tags`: the tags with their number of memos
  7. `glance`: the app glance in the launcher with the number of open todos
* `banner-720x320.png`: header banner
* `settings/`: screenshots of the settings page for the README, one per section; regenerate them with `node store/settings_screenshots.js`
* Take the screenshots again with `store/take_screenshots.py` (see the README)

Regenerate the graphics, and the menu icons in `resources/`, with `python3 store/make_art.py`.
