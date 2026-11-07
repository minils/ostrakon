// Integration test of src/pkjs/index.js against a real Memos server, to catch
// changes of the Memos API that the mock server in pkjs.test.js cannot.
// Needs a fresh Memos server without users, e.g. from Docker:
//   docker run -d -p 5230:5230 neosmemo/memos:stable
//   MEMOS_URL=http://localhost:5230 node test/memos.test.js
// Run with PKJS_BUNDLE=build/pebble-js-app.js to test the built bundle.
process.env.TZ = 'UTC';
const assert = require('assert');
const path = require('path');

const MEMOS_URL =
    (process.env.MEMOS_URL || 'http://localhost:5230').replace(/\/+$/, '');

/* ---------- PebbleKit JS stubs, as in pkjs.test.js ---------- */
const handlers = {};
const emit = (name, event) => (handlers[name] || []).forEach(f => f(event));
const received = [];
const store = {};
global.localStorage = {
  getItem: k => store[k] ?? null,
  setItem: (k, v) => store[k] = String(v)
};
global.Pebble = {
  addEventListener: (n, f) => (handlers[n] = handlers[n] || []).push(f),
  getAccountToken: () => 'account',
  getWatchToken: () => 'watch',
  getActiveWatchInfo: () => ({
    platform: 'emery',
    model: 'pebble_time_2',
    language: 'en_US',
    firmware: {major: 4, minor: 4, patch: 0, suffix: ''}
  }),
  platform: 'android',
  sendAppMessage: (msg, ok) => setTimeout(
      () => {
        received.push(msg);
        ok({});
      },
      1),
  openURL: () => {},
};
// every request of the app, to check which API paths it uses
const requests = [];
global.XMLHttpRequest = class {
  open(method, url) {
    this.method = method;
    this.url = url;
    this.headers = {};
  }
  setRequestHeader(k, v) {
    this.headers[k] = v;
  }
  send(body) {
    fetch(
        this.url,
        {method: this.method, headers: this.headers, body: body || undefined})
        .then(r => r.text().then(t => {
          requests.push([this.method, new URL(this.url).pathname, r.status]);
          this.status = r.status;
          this.responseText = t;
          this.onload();
        }))
        .catch(() => this.onerror());
  }
};
const Module = require('module');
const originalLoad = Module._load;
Module._load = function(request, ...rest) {
  return request === 'message_keys' ? {} :
                                      originalLoad.call(this, request, ...rest);
};
require(
    process.env.PKJS_BUNDLE ?
        path.resolve(process.env.PKJS_BUNDLE) :
        path.join(__dirname, '..', 'src', 'pkjs', 'index.js'));

const ACTION = {
  CREATE: 1,
  LIST_RECENT: 2,
  LIST_TAGS: 3,
  LIST_BY_TAG: 4,
  GET: 5,
  ARCHIVE: 6,
  DELETE: 7,
  COMMENT: 8,
  LIST_TASKS: 9,
  TOGGLE_TASK: 10,
  APPEND: 11,
  SEARCH: 12,
  LIST_PINNED: 13,
  TOGGLE_PIN: 14,
  LIST_COMMENTS: 15
};

let reqId = 0;
// A request from the watch; resolves to [head, ...list items]
function request(action, target, text) {
  const id = ++reqId;
  const before = received.length;
  emit(
      'appmessage',
      {payload: {REQUEST_ID: id, ACTION: action, ITEM_ID: target, TEXT: text}});
  return new Promise(resolve => {
    const t = setInterval(() => {
      const msgs = received.slice(before).filter(m => m.REQUEST_ID === id);
      const head = msgs[0];
      if (!head) return;
      const items = [];
      for (const m of msgs) {
        if (m.ITEMS === undefined) continue;
        for (const record of m.ITEMS.split('\x1e')) {
          const [id, title, subtitle, flags] = record.split('\x1f');
          items.push({
            ITEM_ID: id,
            ITEM_TITLE: title,
            ITEM_SUBTITLE: subtitle,
            ITEM_FLAGS: +flags
          });
        }
      }
      if (head.STATUS !== 0 || head.LIST_COUNT === undefined ||
          items.length === head.LIST_COUNT) {
        clearInterval(t);
        resolve([head, ...items]);
      }
    }, 5);
  });
}

// The notice the watch gets after the settings are saved
async function savedNotice(url, token) {
  const values = {
    SETTING_URL: {value: url},
    SETTING_TOKEN: {value: token},
    SETTING_TAGS: {value: ''},
    SETTING_TODO_TAG: {value: ''},
    VIBRATE: {value: true},
    SETTING_LANGUAGE: {value: 'en'}
  };
  const start = received.length;
  emit('webviewclosed', {response: encodeURIComponent(JSON.stringify(values))});
  for (let i = 0; i < 200; i++) {
    const notice = received.slice(start).find(m => 'NOTICE' in m);
    if (notice) return notice;
    await new Promise(r => setTimeout(r, 25));
  }
  throw new Error('No notice after saving the settings');
}

/* ---------- setup of the Memos server ---------- */
async function waitForServer() {
  for (let i = 0; i < 90; i++) {
    try {
      if ((await fetch(MEMOS_URL + '/healthz')).ok) return;
    } catch (err) {
    }
    await new Promise(r => setTimeout(r, 1000));
  }
  throw new Error('Memos at ' + MEMOS_URL + ' did not become ready');
}

async function call(api, method, apiPath, body, token) {
  const headers = {'Content-Type': 'application/json'};
  if (token) headers.Authorization = 'Bearer ' + token;
  const r = await fetch(
      api + apiPath,
      {method, headers, body: body ? JSON.stringify(body) : undefined});
  const text = await r.text();
  if (!r.ok) throw new Error(method + ' ' + apiPath + ': ' + r.status + text);
  return text ? JSON.parse(text) : {};
}

// The first user becomes the admin; the app gets a personal access token,
// as users are told to create in the settings
async function createToken() {
  // Memos 26.10 serves the API at /api, older versions at /api/v1
  const current = await fetch(MEMOS_URL + '/api/instance/profile');
  const api = MEMOS_URL + (current.ok ? '/api' : '/api/v1');
  const user = await call(api, 'POST', '/users', {
    username: 'ostrakon',
    password: 'ostrakon-password',
    email: 'ostrakon@example.test'
  });
  const signIn = await call(api, 'POST', '/auth/signin', {
    passwordCredentials: {username: 'ostrakon', password: 'ostrakon-password'}
  });
  const pat = await call(
      api, 'POST', '/' + user.name + '/personalAccessTokens',
      {description: 'Ostrakon integration test'}, signIn.accessToken);
  assert.ok(pat.token, 'no token in ' + JSON.stringify(pat));
  return pat.token;
}

setTimeout(() => {
  console.error('Test timed out');
  process.exit(1);
}, 120000).unref();

(async () => {
  await waitForServer();
  const profile = await (await fetch(MEMOS_URL + '/api/instance/profile'))
                      .json()
                      .catch(() => null);
  console.log('Memos', profile ? profile.version : '< 26.10');
  const token = await createToken();

  // the settings check the connection right away
  assert.deepStrictEqual(
      await savedNotice(MEMOS_URL, 'memos_pat_wrong'),
      {NOTICE: 'Token rejected (401)', STATUS: 1});
  assert.deepStrictEqual(
      await savedNotice(MEMOS_URL, token),
      {NOTICE: 'Connected to Memos', STATUS: 0});

  // create: the default tag is added
  let r = await request(ACTION.CREATE, null, 'Integration test memo');
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);
  // create times are in seconds; the second memo is to be the newer one
  await new Promise(res => setTimeout(res, 1100));
  r = await request(ACTION.CREATE, null, 'Second memo with a task');
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);

  // recent: newest first
  r = await request(ACTION.LIST_RECENT);
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);
  assert.deepStrictEqual(
      r.slice(1).map(m => m.ITEM_TITLE),
      ['Second memo with a task', 'Integration test memo']);
  const [second, first] = r.slice(1).map(m => m.ITEM_ID);
  assert.match(first, /^memos\//);

  // a single memo with its content
  r = await request(ACTION.GET, first);
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);
  assert.ok(r[0].TEXT.includes('Integration test memo'), r[0].TEXT);

  // tags, as the server extracts them, and memos by tag
  r = await request(ACTION.LIST_TAGS);
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);
  assert.deepStrictEqual(
      r.slice(1).map(m => [m.ITEM_ID, m.ITEM_SUBTITLE]),
      [['pebble', '2 memos']]);
  r = await request(ACTION.LIST_BY_TAG, 'pebble');
  assert.strictEqual(r[0].LIST_COUNT, 2);

  // search
  r = await request(ACTION.SEARCH, null, 'integration');
  assert.deepStrictEqual(r.slice(1).map(m => m.ITEM_ID), [first]);

  // comments: written, listed with their author, and marked on the memo
  r = await request(ACTION.COMMENT, first, 'A comment from the watch');
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);
  r = await request(ACTION.LIST_COMMENTS, first);
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);
  assert.deepStrictEqual(
      r.slice(1).map(m => m.ITEM_TITLE), ['A comment from the watch']);
  assert.ok(r[1].ITEM_SUBTITLE.startsWith('ostrakon'), r[1].ITEM_SUBTITLE);
  const comment = r[1].ITEM_ID;
  r = await request(ACTION.LIST_RECENT);
  assert.deepStrictEqual(
      r.slice(1).map(m => m.ITEM_ID), [second, first],
      'comments are not listed as memos');
  assert.ok(r[2].ITEM_FLAGS & 2, 'the memo is marked as having comments');

  // append a task and tick it off
  r = await request(ACTION.APPEND, second, 'check box buy milk');
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);
  r = await request(ACTION.LIST_TASKS, second);
  assert.deepStrictEqual(
      r.slice(1).map(m => [m.ITEM_ID, m.ITEM_TITLE, m.ITEM_FLAGS]),
      [[second + '#0', 'buy milk', 0]]);
  r = await request(ACTION.TOGGLE_TASK, second + '#0', 'buy milk');
  assert.deepStrictEqual([r[0].STATUS, r[0].ITEM_FLAGS], [0, 1], r[0].TEXT);
  r = await request(ACTION.LIST_TASKS, second);
  assert.strictEqual(r[1].ITEM_FLAGS, 1);

  // pin
  r = await request(ACTION.TOGGLE_PIN, first);
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [0, 'Pinned']);
  r = await request(ACTION.LIST_PINNED);
  assert.deepStrictEqual(r.slice(1).map(m => m.ITEM_ID), [first]);
  r = await request(ACTION.TOGGLE_PIN, first);
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [0, 'Unpinned']);

  // archive a comment and a memo, delete a memo
  r = await request(ACTION.ARCHIVE, comment);
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);
  r = await request(ACTION.LIST_COMMENTS, first);
  assert.strictEqual(r[0].LIST_COUNT, 0);
  r = await request(ACTION.ARCHIVE, first);
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);
  r = await request(ACTION.DELETE, second);
  assert.strictEqual(r[0].STATUS, 0, r[0].TEXT);
  r = await request(ACTION.LIST_RECENT);
  assert.strictEqual(r[0].LIST_COUNT, 0);
  r = await request(ACTION.GET, second);
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Server error 404']);

  // the app ends up on the API path this server serves
  const paths = requests.map(([, p]) => p);
  console.log(
      'API paths used:',
      [...new Set(paths.map(p => p.replace(/(api\/(v1\/)?).*/, '$1')))]);
  assert.ok(!requests.some(([, , status]) => status >= 500), 'server errors');

  console.log('ALL OK');
})().catch(e => {
  console.error(e);
  process.exit(1);
});
