// End-to-end test of src/pkjs/index.js against a mock Memos server.
// Stubs the PebbleKit JS environment and simulates watch requests.
// Run with: node test/pkjs.test.js (needs Node 18+ for fetch)
process.env.TZ = 'UTC';  // dates in list subtitles are local time
const http = require('http');
const assert = require('assert');
const path = require('path');

/* ---------- mock Memos server ---------- */
let memos = [];
for (let i = 1; i <= 60; i++) {
  memos.push({
    name: 'memos/m' + i,
    content:
        (i % 3 === 0 ? '# Title ' + i + '\nbody #work' :
                       '- [ ] task ' + i + ' #home'),
    tags: i % 3 === 0 ? ['work'] : ['home'],
    createTime: '2026-10-1' + (i % 9) + 'T12:0' + (i % 6) + ':00Z',
    state: 'NORMAL'
  });
}
memos.push({
  name: 'memos/p1',
  content: 'project note',
  tags: ['proj/a'],
  createTime: '2026-10-18T09:00:00Z',
  state: 'NORMAL'
});
memos.push({
  name: 'memos/md1',
  tags: ['idea'],
  createTime: '2026-10-01T09:00:00Z',
  state: 'NORMAL',
  content: [
    '# Big title', '', '## Sub *title*',
    'Some **bold** and `code` with [a link](http://x.y).', '- [ ] open task',
    '- [x] done task', '* bullet', '1. first', '> quoted', '---', '```',
    'let x = 1;', '- [ ] in code', '```', '![pic](http://img)', '#idea #pebble'
  ].join('\n')
});
memos.push({
  name: 'memos/c1',
  content: 'a comment',
  parent: 'memos/m1',
  tags: [],
  state: 'NORMAL'
});
memos.push({
  name: 'memos/c2',
  content: 'second **reply**',
  parent: 'memos/m3',
  creator: 'users/1',
  tags: [],
  createTime: '2026-10-17T10:00:00Z',
  state: 'NORMAL'
});
memos.push({
  name: 'memos/c3',
  content: 'first reply',
  parent: 'memos/m3',
  creator: 'users/alice',
  tags: [],
  createTime: '2026-10-16T10:00:00Z',
  state: 'NORMAL'
});
memos.forEach(x => {
  if (!x.parent) x.relations = [];
});
memos.find(x => x.name === 'memos/m3').relations = [
  {memo: {name: 'memos/c2'}, relatedMemo: {name: 'memos/m3'}, type: 'COMMENT'},
  {memo: {name: 'memos/c3'}, relatedMemo: {name: 'memos/m3'}, type: 'COMMENT'}
];
// a reference from m4 to m3 is not a comment of m4
memos.find(x => x.name === 'memos/m4').relations = [
  {memo: {name: 'memos/m4'}, relatedMemo: {name: 'memos/m3'}, type: 'REFERENCE'}
];
memos.find(x => x.name === 'memos/m5').pinned = true;
memos.find(x => x.name === 'memos/m7').pinned = true;
memos.push({
  name: 'memos/u1',
  content: 'Ünïcödé 😀 ' +
      'x'.repeat(2000),
  tags: ['work/sub'],
  createTime: '2026-10-18T08:00:00Z',
  state: 'NORMAL'
});
const created = [], comments = [];
let serverDown = false;
let userRequests = 0;
const server = http.createServer((req, res) => {
  if (serverDown) return req.socket.destroy();
  let body = '';
  req.on('data', c => body += c);
  req.on('end', () => {
    if (req.headers.authorization !== 'Bearer tok') {
      res.writeHead(401);
      return res.end('{}');
    }
    const u = new URL(req.url, 'http://x');
    const send = (code, obj) => {
      res.writeHead(code, {'Content-Type': 'application/json'});
      res.end(JSON.stringify(obj));
    };
    let m;
    if (req.method === 'GET' && u.pathname === '/api/v1/memos') {
      const size = +u.searchParams.get('pageSize'),
            start = +(u.searchParams.get('pageToken') || 0);
      const visible = memos.filter(x => x.state !== 'ARCHIVED');
      const page = visible.slice(start, start + size);
      return send(200, {
        memos: page,
        nextPageToken: start + size < visible.length ? String(start + size) : ''
      });
    }
    if (req.method === 'POST' && u.pathname === '/api/v1/memos') {
      created.push(JSON.parse(body));
      const memo = {
        name: 'memos/new' + created.length,
        content: JSON.parse(body).content,
        createTime: '2026-10-18T10:00:00Z',
        state: 'NORMAL'
      };
      memos.unshift(memo);
      return send(200, memo);
    }
    if ((m = u.pathname.match(/^\/api\/v1\/(memos\/[\w]+)\/comments$/)) &&
        req.method === 'POST') {
      comments.push([m[1], JSON.parse(body)]);
      return send(200, {});
    }
    if ((m = u.pathname.match(/^\/api\/v1\/(memos\/[\w]+)\/comments$/)) &&
        req.method === 'GET') {
      return send(200, {memos: memos.filter(x => x.parent === m[1])});
    }
    if (req.method === 'GET' && u.pathname === '/api/v1/users/1') {
      userRequests++;
      return send(
          200, {name: 'users/1', username: 'nils', displayName: 'Nils'});
    }
    if ((m = u.pathname.match(/^\/api\/v1\/(memos\/[\w]+)$/))) {
      const memo = memos.find(x => x.name === m[1]);
      if (!memo) return send(404, {});
      if (req.method === 'GET') return send(200, memo);
      if (req.method === 'DELETE') {
        memos = memos.filter(x => x !== memo);
        return send(200, {});
      }
      if (req.method === 'PATCH') {
        const mask = u.searchParams.get('updateMask');
        assert.ok(['state', 'content', 'pinned'].includes(mask), mask);
        memo[mask] = JSON.parse(body)[mask];
        return send(200, memo);
      }
    }
    send(404, {});
  });
});

/* ---------- PebbleKit JS stubs ---------- */
// events can have several listeners, e.g. Clay adds one for "ready"
const handlers = {};
const listeners = new Proxy({}, {
  get: (_, name) => event => (handlers[name] || []).forEach(f => f(event)),
});
const received = [];
let nackNext = false;
const store = {};
const openedUrls = [];
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
  sendAppMessage: (msg, ok, fail) => setTimeout(
      () => {
        if (nackNext) {
          nackNext = false;
          return fail({});
        }
        received.push(msg);
        ok({});
      },
      1),
  openURL: url => {
    openedUrls.push(url);
  },
};
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
          this.status = r.status;
          this.responseText = t;
          this.onload();
        }))
        .catch(() => this.onerror());
  }
};
// the Pebble build provides the message_keys module, which Clay loads
const Module = require('module');
const originalLoad = Module._load;
Module._load = function(request, ...rest) {
  return request === 'message_keys' ? {} :
                                      originalLoad.call(this, request, ...rest);
};
require(path.join(__dirname, '..', 'src', 'pkjs', 'index.js'));

let reqId = 0;
// What the Clay settings page sends back when it is saved
function clayResponse(s) {
  return encodeURIComponent(JSON.stringify({
    SETTING_URL: {value: s.url},
    SETTING_TOKEN: {value: s.token},
    SETTING_TAGS: {value: s.tags || ''},
    SETTING_TODO_TAG: {value: s.todoTag || ''},
    VIBRATE: {value: s.vibrate !== false},
    SETTING_LANGUAGE: {value: s.language || 'auto'}
  }));
}

// List items arrive packed into few messages; unpack them into one object
// per item, checking the packing on the way
function decodeItems(msgs) {
  const items = [];
  for (const m of msgs) {
    if (m.ITEMS === undefined) continue;
    assert.strictEqual(m.ITEM_INDEX, items.length);
    assert.ok(Buffer.byteLength(m.ITEMS) <= 1300);
    for (const record of m.ITEMS.split('\x1e')) {
      const [id, title, subtitle, flags] = record.split('\x1f');
      items.push({
        ITEM_INDEX: items.length,
        ITEM_ID: id,
        ITEM_TITLE: title,
        ITEM_SUBTITLE: subtitle,
        ITEM_FLAGS: +flags
      });
    }
  }
  return items;
}

function request(action, target, text, offset) {
  const id = ++reqId;
  const before = received.length;
  listeners.appmessage({
    payload: {
      REQUEST_ID: id,
      ACTION: action,
      ITEM_ID: target,
      TEXT: text,
      ITEM_INDEX: offset
    }
  });
  return new Promise(resolve => {
    const t = setInterval(() => {
      const msgs = received.slice(before).filter(m => m.REQUEST_ID === id);
      const head = msgs[0];
      if (!head) return;
      const items = decodeItems(msgs);
      if (head.STATUS !== 0 || head.LIST_COUNT === undefined ||
          items.length === head.LIST_COUNT) {
        clearInterval(t);
        resolve([head, ...items]);
      }
    }, 5);
  });
}
const bytes = s => Buffer.byteLength(s, 'utf8');

// fail instead of hanging if a request never gets an answer
setTimeout(() => {
  console.error('Test timed out');
  process.exit(1);
}, 30000).unref();

(async () => {
  await new Promise(r => server.listen(0, r));
  const port = server.address().port;

  // not configured
  let r = await request(2);
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Not configured']);

  // legacy URL form with /api/v1/memos is accepted
  store.ostrakon_settings = JSON.stringify({
    url: `http://127.0.0.1:${port}/api/v1/memos`,
    token: 'tok',
    tags: '#pebble #watch'
  });

  // recent: 20 items, comments skipped, limits respected
  r = await request(2);
  assert.strictEqual(r[0].LIST_COUNT, 20);
  const items = r.slice(1);
  assert.ok(items.every(m => m.ITEM_ID !== 'memos/c1'));
  assert.ok(items.every(
      (m, i) => m.ITEM_INDEX === i && bytes(m.ITEM_TITLE) <= 63 &&
          bytes(m.ITEM_SUBTITLE) <= 47));
  assert.strictEqual(items[0].ITEM_TITLE, 'task 1');
  assert.strictEqual(items[2].ITEM_TITLE, 'Title 3');
  console.log('recent ok:', items[0], items[2].ITEM_TITLE);

  // tags with counts, hierarchical tag counted
  r = await request(3);
  const tags = r.slice(1).map(m => [m.ITEM_ID, m.ITEM_TITLE, m.ITEM_SUBTITLE]);
  console.log('tags:', tags);
  assert.deepStrictEqual(tags[0], ['home', '#home', '40 memos']);

  // by tag: across pages, limited to 20; "proj" also matches "proj/a"
  r = await request(4, 'work');
  assert.strictEqual(r[0].LIST_COUNT, 20);
  assert.ok(r.slice(1).every(m => +m.ITEM_ID.slice(7) % 3 === 0));
  r = await request(4, 'proj');
  assert.deepStrictEqual(r.slice(1).map(m => m.ITEM_ID), ['memos/p1']);
  r = await request(4, 'nothing');
  assert.strictEqual(r[0].LIST_COUNT, 0);

  // get: utf-8 safe truncation to 1500 bytes
  r = await request(5, 'memos/u1');
  assert.strictEqual(r[0].STATUS, 0);
  assert.ok(bytes(r[0].TEXT) <= 1500 && r[0].TEXT.startsWith('pÜnïcödé 😀'));
  assert.ok(!/�/.test(Buffer.from(r[0].TEXT).toString()));

  // markdown is converted into typed lines for the watch
  r = await request(5, 'memos/md1');
  assert.deepStrictEqual(r[0].TEXT.split('\n'), [
    '1Big title', '2Sub title', 'pSome bold and code with a link.',
    'copen task', 'xdone task', 'bbullet', 'n1. first', 'qquoted', '-',
    'klet x = 1;', 'k- [ ] in code', 'p[pic]', 't#idea #pebble'
  ]);

  // tasks of one memo in document order, ids are memo name and task index;
  // checkboxes in code blocks are not tasks
  r = await request(9, 'memos/md1');
  assert.deepStrictEqual(
      r.slice(1).map(t => [t.ITEM_ID, t.ITEM_TITLE, t.ITEM_FLAGS]),
      [['memos/md1#0', 'open task', 0], ['memos/md1#1', 'done task', 1]]);
  r = await request(9, 'memos/m1');
  assert.deepStrictEqual(r.slice(1).map(t => t.ITEM_TITLE), ['task 1']);
  r = await request(9, 'memos/u1');
  assert.strictEqual(r[0].LIST_COUNT, 0);
  r = await request(9, 'memos/nope');
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Server error 404']);

  // toggling patches the memo content; code blocks are not tasks
  r = await request(10, 'memos/md1#0', 'open task');
  assert.deepStrictEqual([r[0].STATUS, r[0].ITEM_FLAGS], [0, 1]);
  let md1 = memos.find(m => m.name === 'memos/md1').content;
  assert.ok(
      md1.includes('- [x] open task') && md1.includes('- [x] done task') &&
      md1.includes('- [ ] in code'));
  r = await request(10, 'memos/md1#1', 'done task');
  assert.deepStrictEqual([r[0].STATUS, r[0].ITEM_FLAGS], [0, 0]);
  md1 = memos.find(m => m.name === 'memos/md1').content;
  assert.ok(md1.includes('- [ ] done task'));
  r = await request(10, 'memos/md1#0', 'something else');
  assert.deepStrictEqual(
      [r[0].STATUS, r[0].TEXT], [1, 'Memo changed, reload tasks']);
  r = await request(10, 'memos/md1#5', 'x');
  assert.strictEqual(r[0].STATUS, 1);
  assert.ok(memos.find(m => m.name === 'memos/md1')
                .content.includes('- [x] open task'));

  // create with keywords and tags; empty memos are rejected
  r = await request(
      1, null, 'title big shopping new line check box milk hashtag Food');
  assert.strictEqual(r[0].STATUS, 0);
  console.log('created:', JSON.stringify(created[0].content));
  assert.strictEqual(
      created[0].content, '# shopping\n- [ ] milk\n\n#pebble #watch #food');
  for (const empty of ['', '   ', 'hashtag work', 'check box', 'title big']) {
    r = await request(1, null, empty);
    assert.deepStrictEqual(
        [r[0].STATUS, r[0].TEXT], [1, 'Memo is empty'], empty);
  }
  assert.strictEqual(created.length, 1);

  // comment
  r = await request(8, 'memos/m2', 'looks good');
  assert.strictEqual(r[0].STATUS, 0);
  assert.deepStrictEqual(comments[0], ['memos/m2', {content: 'looks good'}]);
  r = await request(8, 'memos/m2', '  ');
  assert.strictEqual(r[0].STATUS, 1);

  // archive and delete remove memos from the recent list
  assert.strictEqual((await request(6, 'memos/m1'))[0].STATUS, 0);
  assert.strictEqual((await request(7, 'memos/m2'))[0].STATUS, 0);
  r = await request(2);
  assert.ok(!r.slice(1).some(
      m => m.ITEM_ID === 'memos/m1' || m.ITEM_ID === 'memos/m2'));
  r = await request(7, 'memos/nope');
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Server error 404']);

  // without a connection lists and memos come from the cache
  serverDown = true;
  r = await request(2);
  assert.strictEqual(r[0].STATUS, 0);
  assert.strictEqual(r[0].TEXT, 'Offline: cached memos');
  assert.strictEqual(r[0].LIST_COUNT, 20);
  assert.ok(!r.slice(1).some(
      m => m.ITEM_ID === 'memos/m1' || m.ITEM_ID === 'memos/m2'));
  r = await request(4, 'idea');
  assert.deepStrictEqual(r.slice(1).map(m => m.ITEM_ID), ['memos/md1']);
  r = await request(3);
  assert.strictEqual(r[0].TEXT, 'Offline: cached memos');
  r = await request(5, 'memos/md1');
  assert.ok(r[0].TEXT.startsWith('1Big title'));
  r = await request(5, 'memos/new1');  // created memos are cached, too
  assert.ok(r[0].TEXT.startsWith('1shopping'));
  r = await request(9, 'memos/md1');
  assert.deepStrictEqual(
      [r[0].TEXT, r[0].LIST_COUNT], ['Offline: cached memos', 2]);
  r = await request(5, 'memos/unknown');
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Network error']);
  r = await request(6, 'memos/m3');
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Network error']);
  localStorage.setItem('ostrakon_cache', '[]');
  r = await request(2);
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Network error']);
  serverDown = false;

  // a NACK is retried and nothing is lost
  nackNext = true;
  r = await request(2);
  assert.strictEqual(r.length, 21);

  // wrong token
  store.ostrakon_settings =
      JSON.stringify({url: `http://127.0.0.1:${port}/`, token: 'bad'});
  r = await request(2);
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Server error 401']);

  // the vibration setting is pushed to the watch on start and on change;
  // it defaults to on for settings saved before the option existed.
  // Saving settings also tests the connection.
  let before = received.length;
  listeners.ready({});
  listeners.webviewclosed(
      {response: clayResponse({url: 'x', token: 'y', vibrate: false})});
  await new Promise(res => setTimeout(res, 100));
  let pushed = received.slice(before);
  // the language setting goes along: 0 = follow the watch
  assert.deepStrictEqual(
      pushed.filter(m => 'VIBRATE' in m),
      [{VIBRATE: 1, LANGUAGE: 0}, {VIBRATE: 0, LANGUAGE: 0}]);
  assert.deepStrictEqual(
      pushed.find(m => 'NOTICE' in m),
      {NOTICE: 'Server not reachable', STATUS: 1});

  const good = {
    url: `http://127.0.0.1:${port}`,
    token: 'tok',
    todoTag: '#home'
  };
  const notice = async settings => {
    const start = received.length;
    listeners.webviewclosed({response: clayResponse(settings)});
    await new Promise(res => setTimeout(res, 100));
    return received.slice(start).find(m => 'NOTICE' in m);
  };
  assert.deepStrictEqual(
      await notice({...good, token: 'bad'}),
      {NOTICE: 'Token rejected (401)', STATUS: 1});
  assert.deepStrictEqual(
      await notice({...good, url: good.url + '/nope'}),
      {NOTICE: 'API not found, check URL', STATUS: 1});
  assert.deepStrictEqual(
      await notice(good), {NOTICE: 'Connected to Memos', STATUS: 0});

  // paging: 20 memos per page, the head says whether there are more
  r = await request(2);
  assert.deepStrictEqual([r[0].LIST_COUNT, r[0].ITEM_FLAGS], [20, 1]);
  const firstPage = r.slice(1).map(m => m.ITEM_ID);
  r = await request(2, null, null, 20);
  assert.deepStrictEqual([r[0].LIST_COUNT, r[0].ITEM_FLAGS], [20, 1]);
  // 20 items need only a few messages
  assert.ok(received.filter(m => m.REQUEST_ID === reqId).length <= 3);
  assert.deepStrictEqual(
      r.slice(1).map(m => m.ITEM_INDEX), [...Array(20).keys()]);
  assert.ok(r.slice(1).every(m => !firstPage.includes(m.ITEM_ID)));
  r = await request(2, null, null, 60);
  assert.strictEqual(r[0].ITEM_FLAGS, 0);
  assert.ok(r[0].LIST_COUNT > 0 && r[0].LIST_COUNT < 20);

  // pinned memos and toggling the pin
  r = await request(13);
  assert.deepStrictEqual(
      r.slice(1).map(m => m.ITEM_ID), ['memos/m5', 'memos/m7']);
  r = await request(14, 'memos/m5');
  assert.deepStrictEqual(
      [r[0].STATUS, r[0].ITEM_FLAGS, r[0].TEXT], [0, 0, 'Unpinned']);
  r = await request(14, 'memos/m4');
  assert.deepStrictEqual(
      [r[0].STATUS, r[0].ITEM_FLAGS, r[0].TEXT], [0, 1, 'Pinned']);
  r = await request(13);
  assert.deepStrictEqual(
      r.slice(1).map(m => m.ITEM_ID), ['memos/m4', 'memos/m7']);

  // search: all words, case-insensitive, also in tags; no comments
  r = await request(12, null, 'Title 3');
  assert.deepStrictEqual(
      r.slice(1).map(m => m.ITEM_ID).slice(0, 3),
      ['memos/m3', 'memos/m30', 'memos/m33']);
  r = await request(12, null, 'PROJECT note');
  assert.deepStrictEqual(r.slice(1).map(m => m.ITEM_ID), ['memos/p1']);
  r = await request(12, null, 'idea');
  assert.deepStrictEqual(r.slice(1).map(m => m.ITEM_ID), ['memos/md1']);
  r = await request(12, null, 'reply');
  assert.strictEqual(r[0].LIST_COUNT, 0);
  r = await request(12, null, '  ');
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Nothing to search']);

  // whether a memo has comments comes with the lists and the memo
  r = await request(2, null, null, 0);
  const flagOf = name => r.slice(1).find(m => m.ITEM_ID === name).ITEM_FLAGS;
  assert.deepStrictEqual(
      [flagOf('memos/m3'), flagOf('memos/m4'), flagOf('memos/m6')], [2, 0, 0]);
  r = await request(5, 'memos/m3');
  assert.deepStrictEqual(
      [r[0].TEXT, r[0].ITEM_FLAGS], ['1Title 3\npbody #work', 2]);
  r = await request(5, 'memos/m4');
  assert.strictEqual(r[0].ITEM_FLAGS, 0);

  // comments are listed separately, oldest first, with author and date;
  // a user without a display name falls back to the name in the creator
  r = await request(15, 'memos/m3');
  assert.deepStrictEqual(
      r.slice(1).map(m => [m.ITEM_ID, m.ITEM_TITLE, m.ITEM_SUBTITLE]), [
        ['memos/c3', 'first reply', 'alice \u00b7 Oct 16, 10:00'],
        ['memos/c2', 'second **reply**', 'Nils \u00b7 Oct 17, 10:00']
      ]);
  r = await request(15, 'memos/m4');
  assert.strictEqual(r[0].LIST_COUNT, 0);

  // a comment opened on its own starts with author and date; user names
  // are only looked up once
  r = await request(5, 'memos/c2');
  assert.deepStrictEqual(
      r[0].TEXT.split('\n'), ['mNils \u00b7 Oct 17, 10:00', 'psecond reply']);
  assert.strictEqual(userRequests, 1);

  // archived comments are not listed
  r = await request(6, 'memos/c3');
  assert.strictEqual(r[0].STATUS, 0);
  r = await request(15, 'memos/m3');
  assert.deepStrictEqual(r.slice(1).map(m => m.ITEM_ID), ['memos/c2']);

  // append: new lines go above the trailing tags
  r = await request(11, 'memos/new1', 'check box eggs');
  assert.strictEqual(r[0].STATUS, 0);
  assert.strictEqual(
      memos.find(m => m.name === 'memos/new1').content,
      '# shopping\n- [ ] milk\n- [ ] eggs\n\n#pebble #watch #food');
  r = await request(11, 'memos/p1', 'second line');
  assert.strictEqual(
      memos.find(m => m.name === 'memos/p1').content,
      'project note\nsecond line');
  r = await request(11, 'memos/p1', ' ');
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Nothing to append']);

  // glance: open tasks of all memos with the todo tag from the settings
  before = received.length;
  listeners.ready({});
  // the glance update waits a moment after start
  await new Promise(res => setTimeout(res, 5400));
  const glance = received.slice(before).find(m => 'GLANCE_COUNT' in m);
  const expectedOpen =
      memos
          .filter(
              m => !m.parent && m.state !== 'ARCHIVED' &&
                  (m.tags || []).includes('home'))
          .reduce(
              (n, m) => n + (m.content.match(/^- \[ \]/gm) || []).length, 0);
  assert.deepStrictEqual(glance, {GLANCE_COUNT: expectedOpen});
  assert.ok(expectedOpen > 30);

  // offline: new memos are queued and sent with the next request
  serverDown = true;
  r = await request(1, null, 'written offline');
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [0, 'Offline: memo queued']);
  r = await request(1, null, 'also offline');
  assert.strictEqual(r[0].TEXT, 'Offline: memo queued');
  assert.strictEqual(
      JSON.parse(localStorage.getItem('ostrakon_queue')).length, 2);
  serverDown = false;
  before = received.length;
  const createdBefore = created.length;
  await request(2);
  await new Promise(res => setTimeout(res, 200));
  assert.deepStrictEqual(
      created.slice(createdBefore).map(c => c.content.split('\n')[0]),
      ['written offline', 'also offline']);
  assert.deepStrictEqual(
      JSON.parse(localStorage.getItem('ostrakon_queue')), []);
  assert.deepStrictEqual(
      received.slice(before).find(m => 'NOTICE' in m),
      {NOTICE: '2 queued memos sent', STATUS: 0});

  // the settings page is Clay's, filled with the stored settings, and
  // saving it stores them in the app's own format
  listeners.showConfiguration({});
  const page = decodeURIComponent(
      openedUrls.pop().replace(/^data:text\/html;charset=utf-8,/, ''));
  assert.ok(page.includes('Ostrakon') && page.includes('Test connection'));
  assert.ok(page.includes(good.url) && page.includes('#home'));
  const saved = JSON.parse(store.ostrakon_settings);
  assert.deepStrictEqual(saved, {
    url: good.url,
    token: 'tok',
    tags: '',
    todoTag: '#home',
    vibrate: true,
    language: 'auto'
  });

  // German: chosen in the settings, or followed from the watch language
  before = received.length;
  listeners.webviewclosed({response: clayResponse({...good, language: 'de'})});
  await new Promise(res => setTimeout(res, 100));
  pushed = received.slice(before);
  assert.deepStrictEqual(
      pushed.find(m => 'VIBRATE' in m), {VIBRATE: 1, LANGUAGE: 2});
  assert.deepStrictEqual(
      pushed.find(m => 'NOTICE' in m),
      {NOTICE: 'Mit Memos verbunden', STATUS: 0});
  r = await request(3);
  assert.match(r.slice(1)[0].ITEM_SUBTITLE, /^\d+ Memos$/);
  r = await request(2);
  assert.match(r[1].ITEM_SUBTITLE, /^1\d\. Okt, \d\d:\d\d/);
  r = await request(15, 'memos/m3');
  assert.strictEqual(r[1].ITEM_SUBTITLE, 'Nils \u00b7 17. Okt, 10:00');
  r = await request(5, 'memos/missing');
  assert.deepStrictEqual([r[0].STATUS, r[0].TEXT], [1, 'Serverfehler 404']);
  r = await request(
      1, null,
      'Überschrift groß Einkauf neue Zeile Checkbox Milch Kontrollkästchen Brot. Hashtag Küche');
  assert.strictEqual(r[0].STATUS, 0);
  assert.strictEqual(
      created[created.length - 1].content,
      '# Einkauf\n- [ ] Milch - [ ] Brot.\n\n#pebble #küche');
  listeners.showConfiguration({});
  const germanPage = decodeURIComponent(
      openedUrls.pop().replace(/^data:text\/html;charset=utf-8,/, ''));
  assert.ok(
      germanPage.includes('Verbindung testen') &&
      germanPage.includes('Sprache'));
  assert.ok(germanPage.includes('Test von dieser Seite nicht m'));

  store.ostrakon_settings = JSON.stringify({...good, language: 'auto'});
  const watchInfo = Pebble.getActiveWatchInfo;
  Pebble.getActiveWatchInfo = () => ({...watchInfo(), language: 'de_DE'});
  r = await request(5, 'memos/missing');
  assert.strictEqual(r[0].TEXT, 'Serverfehler 404');
  Pebble.getActiveWatchInfo = watchInfo;
  r = await request(5, 'memos/missing');
  assert.strictEqual(r[0].TEXT, 'Server error 404');

  console.log('ALL OK');
  server.close();
})().catch(e => {
  console.error(e);
  process.exit(1);
});
