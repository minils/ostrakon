<p align="center">
  <img src="store/banner-720x320.png" alt="Ostrakon" width="720">
</p>

## <img src="store/icon-48.png" alt="" width="32" height="32" align="top"> Ostrakon

A Pebble watch app for your self-hosted [Memos](https://github.com/usememos/memos) server: dictate new memos, browse and search the ones you have, tick off tasks and add comments, all from your wrist.

An *ostrakon* is a piece of broken pottery that people in antiquity used to write short notes on, which is also where the app icon comes from.

<p align="center">
  <img src="store/screenshots/basalt-1-recent.png" alt="Recent memos" width="144">
  <img src="store/screenshots/basalt-2-memo.png" alt="A memo" width="144">
  <img src="store/screenshots/basalt-3-tasks.png" alt="Tasks of a memo" width="144">
  <img src="store/screenshots/basalt-4-comments.png" alt="Comments with author and time" width="144">
  <img src="store/screenshots/basalt-5-comment.png" alt="A comment" width="144">
  <img src="store/screenshots/basalt-6-tags.png" alt="Tags with their number of memos" width="144">
</p>

<p align="center">
  <img src="store/screenshots/chalk-2-memo.png" alt="A memo" width="180">
  <img src="store/screenshots/chalk-4-comments.png" alt="Comments with author and time" width="180">
  <img src="store/screenshots/emery-2-memo.png" alt="A memo" width="200">
  <img src="store/screenshots/emery-4-comments.png" alt="Comments with author and time" width="200">
</p>

### Origin

Ostrakon started as a fork of [pebble-memos](https://github.com/erimow/pebble-memos) by [Erik Morris (erimow)](https://github.com/erimow), which inspired it; the code has since been rewritten. Thank you, Erik, for the idea of sending a dictated note straight from the watch to Memos!

### Features

**Writing**

* **New**: dictation starts right away. Dictate more, send or discard from the Select menu.
* **Drafts**: unsent memos are kept on the watch (up to three) and work without a phone connection. A memo that cannot reach the phone is saved as a draft automatically.
* **Offline queue**: if the phone has no internet, new memos are queued and sent as soon as it is back online.
* **Append** dictated text to an existing memo, or add a **Comment**.

**Reading**

* **Recent**, **Pinned** and **Tags** lists, 20 memos per page with *Load more*.
* **Search** by dictating a few words; the results show the search term in the title bar.
* Memos are shown with their **markdown** rendered: headings, task checkboxes, bullets, quotes, code and tags.
* **Comments** of a memo can be opened from its menu if it has any. They are listed with author and time; each comment has its own menu to open, append to, archive or delete it, or to add a new comment.
* Recently loaded memos are cached on the phone, so lists and memos also open without internet.

**Organizing**

* **Tasks**: list the checkboxes of a memo and tick them off; the change is saved to the server.
* **Pin / Unpin**, **Archive** and **Delete** (with confirmation).
* The **app glance** in the launcher shows the number of open tasks in all memos with your to-do tag.

**Languages**

The app speaks English and German. By default it follows the language of the watch; the language can also be chosen in the settings.

**Watch support**

Built for all current Pebble platforms, including the new Core Devices watches: basalt, chalk, diorite, emery (Pebble Time 2), flint (Pebble 2 Duo) and gabbro (Pebble Round 2).

### Spoken keywords

Some spoken phrases are turned into markdown when a memo is sent:

| Say (English) | Say (German) | Result |
| --- | --- | --- |
| checkbox, check box | Checkbox, Kontrollkästchen | a task, `- [ ] ` |
| title big / medium / small | Überschrift (or Titel) groß / mittel / klein | a heading, `#`, `##` or `###` |
| new line | neue Zeile | a line break |
| hashtag *word* | Hashtag *Wort* | the tag `#word` |

### Settings

Open the app settings in the Pebble app on your phone:

* **Memos Server URL**, e.g. `https://memos.example.com`
* **Access Token**, created in the Memos settings
* **Test connection** checks URL and token. After saving, the watch also shows whether the connection works.
* **Default Tags** added to every new memo, e.g. `#pebble`
* **To-do Tag** whose open tasks the app glance counts, `#todo` by default
* **Vibrate** on success and errors
* **Language**: automatic (the watch language), English or Deutsch

### Development

The app has two parts: the watch app in C (`src/c`) and the phone part in PebbleKit JS (`src/pkjs`), which talks to the Memos API.

* Build with the [Pebble SDK](https://developer.repebble.com): `pebble build`
* Run the phone-side tests against a mock Memos server: `node test/pkjs.test.js`
* Sources are formatted with `clang-format` in Google style (see `.clang-format`); CI checks this
* The settings page is built with [Clay](https://github.com/pebble/clay) (MIT license), included in `src/pkjs/vendor`
* GitHub Actions runs the tests and builds the app on every push; the `.pbw` can be downloaded from the run's artifacts and installed from the phone.

### License

Ostrakon is released under the [MIT License](LICENSE). The bundled [Clay](https://github.com/pebble/clay) library is MIT-licensed as well, see `src/pkjs/vendor/CLAY-LICENSE`.
