// Takes screenshots of the settings page, one per section, with demo data:
// store/settings/<language>-1-connection.png, -2-memos.png and -3-watch.png
// Usage: node store/settings_screenshots.js [language]   (needs Playwright
// with Chromium; the language is en by default)
const path = require('path');
const Module = require('module');
const {chromium} = require('playwright');

const language = process.argv[2] || 'en';
const out = path.join(__dirname, 'settings');

// a stub of the PebbleKit JS environment, as in test/pkjs.test.js
const store = {
  ostrakon_settings: JSON.stringify({
    url: 'https://memos.example.com',
    token: 'demo-token',
    tags: '#pebble',
    todoTag: '#todo',
    vibrate: true,
    language: language
  })
};
global.localStorage = {
  getItem: key => key in store ? store[key] : null,
  setItem: (key, value) => store[key] = String(value)
};
const listeners = {};
let pageUrl;
global.Pebble = {
  addEventListener: (name, fn) => listeners[name] = fn,
  openURL: url => pageUrl = url,
  sendAppMessage: () => {},
  getActiveWatchInfo: () => ({platform: 'emery', language: 'en_US'}),
  appGlanceReload: () => {}
};
global.XMLHttpRequest = function() {
  this.open = this.setRequestHeader = this.send = () => {};
};
// the Pebble build provides the message_keys module, which Clay loads
const originalLoad = Module._load;
Module._load = function(request, ...rest) {
  return request === 'message_keys' ? {} :
                                      originalLoad.call(this, request, ...rest);
};
console.log = () => {};
require(path.join(__dirname, '..', 'src', 'pkjs', 'index.js'));
listeners.showConfiguration({});

(async () => {
  const browser = await chromium.launch();
  // the size of a phone
  const page = await browser.newPage(
      {viewport: {width: 360, height: 740}, deviceScaleFactor: 2});
  await page.goto(pageUrl);
  await page.waitForSelector('.keyword-row');
  const sections = await page.$$('.section');
  const names = ['connection', 'memos', 'watch'];
  for (let i = 0; i < names.length; i++) {
    await sections[i].screenshot(
        {path: path.join(out, `${language}-${i + 1}-${names[i]}.png`)});
  }
  await browser.close();
})();
