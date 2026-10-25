// A stand-in for a Memos server with the demo data of the screenshots.
// It answers the requests of src/pkjs/index.js, keeps changes in memory
// and needs nothing but Node.
// Usage: node store/demo_server.js [port]   (default 5230)
'use strict';

var http = require('http');

var PORT = Number(process.argv[2]) || 5230;

var USERS = {
  'users/alex': {name: 'users/alex', displayName: 'Alex'},
  'users/sam': {name: 'users/sam', displayName: 'Sam'},
  'users/jordan': {name: 'users/jordan', displayName: 'Jordan'}
};

function memo(id, time, content, extra) {
  var m = {
    name: 'memos/' + id,
    creator: 'users/alex',
    createTime: time,
    displayTime: time,
    state: 'NORMAL',
    pinned: false,
    content: content,
    relations: []
  };
  Object.keys(extra || {}).forEach(function(key) {
    m[key] = extra[key];
  });
  return m;
}

// newest first, as the server lists them; times are UTC like the emulator
var memos = [
  memo('alps', '2025-10-18T08:12:00Z',
       '# Weekend in the Alps\n' +
       '- [x] Book train tickets\n' +
       '- [ ] Pack hiking boots\n' +
       '- [ ] Download offline maps\n' +
       '- [ ] Check hut opening times\n\n' +
       'We meet at the station at **7:30**. The hut has no Wi-Fi, ' +
       'so the maps have to be on the phone.\n\n' +
       '#travel #todo',
       {pinned: true}),
  memo('shopping', '2025-10-18T07:45:00Z',
       'Shopping\n' +
       '- [ ] Oat milk\n' +
       '- [ ] Coffee beans\n' +
       '- [x] Bread\n' +
       '- [ ] Tomatoes\n\n' +
       '#home #todo'),
  memo('anna', '2025-10-17T16:30:00Z',
       'Call with Anna: release on Friday\n\n' +
       'She writes the changelog, I prepare the screenshots. ' +
       'The beta testers get a build tonight.\n\n' +
       '> Small steps, but every week.\n\n' +
       '#work'),
  memo('plants', '2025-10-17T09:05:00Z',
       'App idea: remind me to water the plants when it has not rained ' +
       'for three days\n\n#ideas'),
  memo('books', '2025-10-16T19:40:00Z',
       '## Books to read\n' +
       '- The Left Hand of Darkness\n' +
       '- Piranesi\n' +
       '- Project Hail Mary\n\n' +
       '#books',
       {pinned: true}),
  memo('shard', '2025-10-15T21:10:00Z',
       'Watchface idea: the time scratched onto a shard of pottery, ' +
       'with a new crack every hour\n\n#ideas')
];

var comments = {
  'memos/alps': [
    memo('alps-1', '2025-10-18T09:02:00Z',
         'I can bring the stove and two mugs', {creator: 'users/sam'}),
    memo('alps-2', '2025-10-18T09:20:00Z',
         'Great! The hut is open until the end of October.'),
    memo('alps-3', '2025-10-18T10:05:00Z', 'Count me in for Saturday.',
         {creator: 'users/jordan'})
  ]
};
Object.keys(comments).forEach(function(parent) {
  comments[parent].forEach(function(comment) {
    comment.parent = parent;
  });
  find(parent).relations = comments[parent].map(function(comment) {
    return {type: 'COMMENT', memo: comment.name, relatedMemo: parent};
  });
});

function find(name) {
  var all = memos.concat.apply(memos, Object.keys(comments).map(function(key) {
    return comments[key];
  }));
  for (var i = 0; i < all.length; i++) {
    if (all[i].name === name) return all[i];
  }
  return null;
}

var nextId = 1;

function handle(method, path, query, body) {
  var match;
  if (method === 'GET' && path === 'memos') {
    return {memos: memos, nextPageToken: ''};
  }
  if (method === 'POST' && path === 'memos') {
    var created = memo('new-' + nextId++, new Date().toISOString(), body.content);
    memos.unshift(created);
    return created;
  }
  if ((match = /^(memos\/[^/]+)\/comments$/.exec(path))) {
    var list = comments[match[1]] = comments[match[1]] || [];
    if (method === 'GET') return {memos: list};
    var comment = memo('c-' + nextId++, new Date().toISOString(), body.content);
    comment.parent = match[1];
    list.push(comment);
    return comment;
  }
  if (/^memos\/[^/]+$/.test(path)) {
    var found = find(path);
    if (!found) return null;
    if (method === 'GET') return found;
    if (method === 'DELETE') {
      memos = memos.filter(function(m) {
        return m.name !== path;
      });
      return {};
    }
    if (method === 'PATCH') {
      (query.updateMask || '').split(',').forEach(function(field) {
        var key = field === 'row_status' ? 'rowStatus' : field;
        if (key in body) found[key] = body[key];
      });
      return found;
    }
  }
  if (method === 'GET' && USERS[path]) return USERS[path];
  return null;
}

http.createServer(function(req, res) {
  var url = new URL(req.url, 'http://localhost');
  var path = url.pathname.replace(/^\/api\/v1\//, '');
  var query = {};
  url.searchParams.forEach(function(value, key) {
    query[key] = value;
  });
  var raw = '';
  req.on('data', function(chunk) {
    raw += chunk;
  });
  req.on('end', function() {
    var body = {};
    try {
      if (raw) body = JSON.parse(raw);
    } catch (e) {}
    var result = handle(req.method, path, query, body);
    console.log(req.method, url.pathname + url.search, result ? 200 : 404);
    res.writeHead(result ? 200 : 404, {'Content-Type': 'application/json'});
    res.end(JSON.stringify(result || {message: 'not found'}));
  });
}).listen(PORT, function() {
  console.log('Demo Memos server on http://localhost:' + PORT);
});
