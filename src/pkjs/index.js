var i18n = require('./i18n');

var SETTINGS_KEY = 'ostrakon_settings';

// Reads a stored JSON value, or null if there is none
function loadStored(key) {
  var raw = localStorage.getItem(key);
  if (raw === null) return null;
  try {
    return JSON.parse(raw);
  } catch (e) {
    return null;
  }
}

function getSettings() {
  return loadStored(SETTINGS_KEY);
}

var DEFAULT_TAGS = ['#pebble'];

/* ---------- ACTIONS (MUST MATCH CommAction IN comm.h) ---------- */
var ACTION_CREATE = 1;
var ACTION_LIST_RECENT = 2;
var ACTION_LIST_TAGS = 3;
var ACTION_LIST_BY_TAG = 4;
var ACTION_GET = 5;
var ACTION_ARCHIVE = 6;
var ACTION_DELETE = 7;
var ACTION_COMMENT = 8;
var ACTION_LIST_TASKS = 9;
var ACTION_TOGGLE_TASK = 10;
var ACTION_APPEND = 11;
var ACTION_SEARCH = 12;
var ACTION_LIST_PINNED = 13;
var ACTION_TOGGLE_PIN = 14;
var ACTION_LIST_COMMENTS = 15;

/* ---------- FLAGS OF MEMO LIST ITEMS (MUST MATCH memo_list.c) ---------- */
var FLAG_HAS_COMMENTS = 2;

/* ---------- LIMITS (MUST FIT THE BUFFERS IN comm.h) ---------- */
var MAX_ID_BYTES = 47;
var MAX_TITLE_BYTES = 63;
var MAX_SUBTITLE_BYTES = 47;
var MAX_TEXT_BYTES = 1500;
var MAX_LIST_ITEMS = 20;
var MAX_TAG_ITEMS = 30;
var MAX_TASK_ITEMS = 30;

var CACHE_KEY = 'ostrakon_cache';
var QUEUE_KEY = 'ostrakon_queue';
var DEFAULT_TODO_TAG = 'todo';
var MAX_CACHED_MEMOS = 200;
// the watch shows at most MAX_TEXT_BYTES, so more is not worth caching
var MAX_CACHED_CONTENT = 2000;
// list items are packed into messages of up to this many bytes
var MAX_BATCH_BYTES = 1200;
// the glance update waits, so it does not slow down the first list
var GLANCE_DELAY_MS = 5000;

var PAGE_SIZE = 50;
var MAX_PAGES = 4;
var TIMEOUT_MS = 15000;

/* =========================================================
 * Language
 * ========================================================= */

// "en" or "de", from the language setting or the watch language
function currentLanguage() {
  return i18n.effectiveLanguage((getSettings() || {}).language);
}

// Translates an English text right before it is sent to the watch
function translate(text) {
  return i18n.translate(text, currentLanguage());
}

/* =========================================================
 * Text helpers
 * ========================================================= */

// Truncate to a maximum number of UTF-8 bytes without splitting a character
function truncateUtf8(text, maxBytes) {
  var bytes = 0;
  for (var i = 0; i < text.length; i++) {
    var code = text.charCodeAt(i);
    var len;
    if (code < 0x80)
      len = 1;
    else if (code < 0x800)
      len = 2;
    else if (code >= 0xD800 && code <= 0xDBFF)
      len = 4;  // surrogate pair
    else
      len = 3;

    if (bytes + len > maxBytes) {
      return text.substring(0, i);
    }
    bytes += len;
    if (len === 4) i++;  // skip the low surrogate
  }
  return text;
}

function formatDate(isoString) {
  if (!isoString) return '';
  var d = new Date(isoString);
  if (isNaN(d.getTime())) return '';
  return i18n.formatDate(d, currentLanguage());
}

/* =========================================================
 * Memo parser
 * ========================================================= */

// Spoken phrases and the markdown they stand for, in English and German
var SPOKEN_MARKDOWN = [
  {phrases: ['check box', 'checkbox', 'kontrollkästchen'], markdown: '- [ ] '},
  {phrases: ['new line', 'neue zeile'], markdown: '\n'},
  {phrases: ['title big', 'überschrift groß', 'titel groß'], markdown: '# '}, {
    phrases: ['title medium', 'überschrift mittel', 'titel mittel'],
    markdown: '## '
  },
  {
    phrases: ['title small', 'überschrift klein', 'titel klein'],
    markdown: '### '
  }
];

// Punctuation the dictation puts after a phrase belongs to the phrase
var SPOKEN_MARKDOWN_PATTERNS = SPOKEN_MARKDOWN.map(function(entry) {
  return {
    regex: new RegExp('(?:' + entry.phrases.join('|') + ')[\\s,.]*', 'gi'),
    markdown: entry.markdown
  };
});

function applyKeywords(text) {
  var content = SPOKEN_MARKDOWN_PATTERNS.reduce(function(result, pattern) {
    return result.replace(pattern.regex, pattern.markdown);
  }, text);
  return content.replace(/[ \t]+\n/g, '\n');
}

// "hashtag kitchen" becomes the tag #kitchen; a tag ends at a space or
// punctuation, so it can contain umlauts
function takeSpokenTags(text) {
  var tags = [];
  var rest = text.replace(/hashtag\s+([^\s,.!?;:]+)/gi, function(phrase, word) {
    tags.push('#' + word.toLowerCase());
    return '';
  });
  return {tags: tags, text: rest.trim()};
}

// Returns the final memo content, or null if there is no actual text
function parseMemo(text, defaultTags) {
  var spoken = takeSpokenTags(applyKeywords(text));
  // only markdown leftovers (e.g. a lone checkbox) count as empty, too
  if (spoken.text.replace(/[-#\[\]\s]/g, '') === '') return null;
  return spoken.text + '\n\n' + defaultTags.concat(spoken.tags).join(' ');
}

function memoTags(memo) {
  if (memo.tags && memo.tags.length) return memo.tags;
  if (memo.property && memo.property.tags) return memo.property.tags;
  var tags = [];
  var regex = /(^|\s)#([^\s#]+)/g;
  var match;
  while ((match = regex.exec(memo.content || '')) !== null) {
    tags.push(match[2]);
  }
  return tags;
}

function hasTag(memo, tag) {
  var tags = memoTags(memo);
  for (var i = 0; i < tags.length; i++) {
    // hierarchical tags like "work/project" also match "work"
    if (tags[i] === tag || tags[i].indexOf(tag + '/') === 0) return true;
  }
  return false;
}

// First line with actual text, without markdown prefixes
function memoTitle(memo) {
  var lines = (memo.content || '').split('\n');
  for (var i = 0; i < lines.length; i++) {
    var line = lines[i]
                   .replace(/^\s*(#+|[-*+]|\d+\.)\s+/, '')
                   .replace(/^\[[ xX]\]\s*/, '')
                   .trim();
    // inline tags are noise in a one-line title
    var withoutTags = line.replace(/(^|\s)#[^\s#]+/g, '').trim();
    if (withoutTags !== '') line = withoutTags;
    if (line !== '') return truncateUtf8(line, MAX_TITLE_BYTES);
  }
  return translate('(empty)');
}

function relationName(value) {
  return typeof value === 'string' ? value : value && value.name;
}

// Whether a memo has comments, from the relations the server lists with
// it. Without relations (older servers) this is unknown, so true.
function hasComments(memo) {
  if (typeof memo.hasComments === 'boolean') return memo.hasComments;
  if (!memo.relations) return true;
  return memo.relations.some(function(relation) {
    return relation.type === 'COMMENT' &&
        relationName(relation.relatedMemo) === memo.name;
  });
}

function memoSubtitle(memo) {
  return truncateUtf8(
      formatDate(memo.displayTime || memo.createTime), MAX_SUBTITLE_BYTES);
}

/* =========================================================
 * Markdown for the watch
 * ========================================================= */

// Removes inline markup the watch cannot show
function inlineMarkdown(text) {
  return text
      .replace(
          /!\[([^\]]*)\]\([^)]*\)/g,
          function(m, alt) {
            return '[' + (alt || 'image') + ']';
          })
      .replace(/\[([^\]]+)\]\([^)]*\)/g, '$1')
      .replace(/`([^`]+)`/g, '$1')
      .replace(/(\*\*|__)(.+?)\1/g, '$2')
      .replace(/~~(.+?)~~/g, '$1')
      .replace(/(^|[\s(])[*_]([^*_\s][^*_]*?)[*_](?=[\s).,!?:;]|$)/g, '$1$2');
}

// Converts markdown into lines of "<type><text>" that the watch renders
// (see memo_view.c): 1/2 heading, p paragraph, b bullet, n numbered,
// c open task, x done task, q quote, t tags, k code, - rule, m meta info
function markdownLines(content) {
  var lines = [];
  var inCode = false;

  content.split('\n').forEach(function(raw) {
    var line = raw.replace(/\s+$/, '');
    var m;
    if (/^\s*```/.test(line)) {
      inCode = !inCode;
    } else if (inCode) {
      lines.push('k' + line);
    } else if (line.trim() === '') {
      // blank lines only separate blocks
    } else if (/^\s*([-*_])(\s*\1){2,}$/.test(line)) {
      lines.push('-');
    } else if ((m = line.match(/^\s*(#{1,6})\s+(.*)$/))) {
      lines.push((m[1].length === 1 ? '1' : '2') + inlineMarkdown(m[2]));
    } else if ((m = line.match(/^\s*[-*+]\s+\[([ xX])\]\s*(.*)$/))) {
      lines.push((m[1] === ' ' ? 'c' : 'x') + inlineMarkdown(m[2]));
    } else if ((m = line.match(/^\s*[-*+]\s+(.*)$/))) {
      lines.push('b' + inlineMarkdown(m[1]));
    } else if ((m = line.match(/^\s*(\d+[.)])\s+(.*)$/))) {
      lines.push('n' + m[1] + ' ' + inlineMarkdown(m[2]));
    } else if ((m = line.match(/^\s*>\s?(.*)$/))) {
      lines.push('q' + inlineMarkdown(m[1]));
    } else if (/^(\s*#[^\s#]+)+$/.test(line)) {
      lines.push('t' + line.trim());
    } else {
      lines.push('p' + inlineMarkdown(line.trim()));
    }
  });

  return lines;
}

// Joins typed lines into the text sent to the watch
function joinLines(lines) {
  if (lines.length === 0) lines = ['p' + translate('(empty)')];
  return truncateUtf8(lines.join('\n'), MAX_TEXT_BYTES);
}

function markdownToLines(content) {
  return joinLines(markdownLines(content));
}

/* =========================================================
 * Tasks (markdown checkboxes)
 * ========================================================= */

var TASK_REGEX = /^(\s*[-*+]\s+\[)([ xX])(\]\s*)(.*)$/;

// Checkbox lines of a memo outside code blocks, in order
function findTasks(content) {
  var tasks = [];
  var inCode = false;
  content.split('\n').forEach(function(line, lineIndex) {
    if (/^\s*```/.test(line)) {
      inCode = !inCode;
      return;
    }
    var m = !inCode && line.match(TASK_REGEX);
    if (m) {
      var text = inlineMarkdown(m[4].trim());
      // inline tags are noise in a one-line task, like in memoTitle
      var withoutTags = text.replace(/(^|\s)#[^\s#]+/g, '').trim();
      tasks.push({
        line: lineIndex,
        done: m[2] !== ' ',
        text: truncateUtf8(
            withoutTags || text || translate('(empty)'), MAX_TITLE_BYTES)
      });
    }
  });
  return tasks;
}

// Flips the checkbox of a task line, returns the new content
function toggleTaskLine(content, lineIndex) {
  var lines = content.split('\n');
  lines[lineIndex] =
      lines[lineIndex].replace(TASK_REGEX, function(m, pre, mark, post, text) {
        return pre + (mark === ' ' ? 'x' : ' ') + post + text;
      });
  return lines.join('\n');
}

// Adds text as new lines at the end of a memo, but above a trailing line
// of tags, so the tags stay at the bottom
function appendToContent(content, text) {
  var lines = content.split('\n');
  var end = lines.length;
  while (end > 0 &&
         (lines[end - 1].trim() === '' ||
          /^(\s*#[^\s#]+)+\s*$/.test(lines[end - 1]))) {
    end--;
  }
  var trailing = lines.slice(end);
  // keep the blank line that separates the tags from the text
  if (trailing.length > 0 && trailing[0].trim() !== '') trailing.unshift('');
  return lines.slice(0, end).concat(text.split('\n'), trailing).join('\n');
}

function isBlankMarkdown(text) {
  return text.replace(/[-#\[\]\s]/g, '') === '';
}

/* =========================================================
 * Offline cache of recently fetched memos
 * ========================================================= */

function memoTime(memo) {
  return Date.parse(memo.displayTime || memo.createTime || '') || 0;
}

function loadCache() {
  try {
    return loadStored(CACHE_KEY) || [];
  } catch (err) {
    return [];
  }
}

function saveCache(memos) {
  try {
    localStorage.setItem(
        CACHE_KEY, JSON.stringify(memos.slice(0, MAX_CACHED_MEMOS)));
  } catch (err) {
    console.log('Ostrakon: Could not save cache', err);
  }
}

function slimMemo(memo) {
  return {
    name: memo.name,
    content: (memo.content || '').substring(0, MAX_CACHED_CONTENT),
    tags: memoTags(memo),
    displayTime: memo.displayTime || memo.createTime,
    parent: memo.parent,
    state: memo.state || memo.rowStatus,
    pinned: !!memo.pinned,
    hasComments: hasComments(memo)
  };
}

// The fetched memos are the newest ones, so they replace the cached memos
// of the same time range; older cached memos are kept unless the server
// returned everything.
function updateCache(fetched, complete) {
  var names = {};
  fetched.forEach(function(memo) {
    names[memo.name] = true;
  });
  var oldest =
      fetched.length ? memoTime(fetched[fetched.length - 1]) : Infinity;
  var older = complete ? [] : loadCache().filter(function(memo) {
    return !names[memo.name] && memoTime(memo) < oldest;
  });
  saveCache(fetched.map(slimMemo).concat(older));
}

function replaceInCache(memo) {
  saveCache(loadCache().map(function(cached) {
    return cached.name === memo.name ? slimMemo(memo) : cached;
  }));
}

function removeFromCache(name) {
  saveCache(loadCache().filter(function(memo) {
    return memo.name !== name;
  }));
}

function findInCache(name) {
  var cached = loadCache();
  for (var i = 0; i < cached.length; i++) {
    if (cached[i].name === name) return cached[i];
  }
  return null;
}

function isOfflineError(err) {
  return err === 'Network error' || err === 'Request timed out';
}

/* =========================================================
 * Outgoing message queue (one message in flight at a time)
 * ========================================================= */
var outbox = [];
var outboxBusy = false;

function enqueue(payload) {
  outbox.push({payload: payload, retries: 0});
  pumpOutbox();
}

function pumpOutbox() {
  if (outboxBusy || outbox.length === 0) return;
  outboxBusy = true;
  var entry = outbox[0];

  Pebble.sendAppMessage(
      entry.payload,
      function() {
        outbox.shift();
        outboxBusy = false;
        pumpOutbox();
      },
      function() {
        entry.retries++;
        if (entry.retries > 2) {
          console.log('Ostrakon: Dropping message after retries');
          outbox.shift();
        }
        outboxBusy = false;
        setTimeout(pumpOutbox, 200);
      });
}

function reply(req, fields) {
  var payload = {'REQUEST_ID': req.requestId, 'ACTION': req.action};
  for (var key in fields) {
    payload[key] = fields[key];
  }
  enqueue(payload);
}

function replyOk(req, fields) {
  fields = fields || {};
  fields['STATUS'] = 0;
  reply(req, fields);
}

function replyError(req, message) {
  console.log('Ostrakon: Request failed:', message);
  reply(
      req,
      {'STATUS': 1, 'TEXT': truncateUtf8(translate(message), MAX_TITLE_BYTES)});
}

function utf8Length(text) {
  return unescape(encodeURIComponent(text)).length;
}

// One item as "id US title US subtitle US flags"; the separators cannot
// appear in the fields
function packItem(item) {
  function field(text, maxBytes) {
    return truncateUtf8((text || '').replace(/[\x1e\x1f]/g, ' '), maxBytes);
  }
  return [
    field(item.id, MAX_ID_BYTES), field(item.title, MAX_TITLE_BYTES),
    field(item.subtitle, MAX_SUBTITLE_BYTES), String(item.flags || 0)
  ].join('\x1f');
}

// Sends the items packed into as few messages as possible, since every
// message costs a round trip to the watch. The first message also carries
// the item count; hasMore marks that another page can be requested.
function replyList(req, items, offline, hasMore) {
  var message = {
    'LIST_COUNT': items.length,
    'ITEM_FLAGS': hasMore ? 1 : 0,
    'STATUS': 0
  };
  if (offline) message['TEXT'] = translate('Offline: cached memos');
  var packed = [];
  var bytes = 0;

  function flush() {
    if (packed.length > 0) {
      message['ITEM_INDEX'] = index - packed.length;
      message['ITEMS'] = packed.join('\x1e');
    }
    reply(req, message);
    message = {};
    packed = [];
    bytes = 0;
  }

  var index = 0;
  for (; index < items.length; index++) {
    var record = packItem(items[index]);
    var size = utf8Length(record) + 1;
    if (packed.length > 0 && bytes + size > MAX_BATCH_BYTES) flush();
    packed.push(record);
    bytes += size;
  }
  flush();
}

// A message for the watch outside of any request, shown as a toast
function notifyWatch(text, success) {
  enqueue({
    'NOTICE': truncateUtf8(translate(text), MAX_TITLE_BYTES),
    'STATUS': success ? 0 : 1
  });
}

/* =========================================================
 * Memos API
 * ========================================================= */

// Accepts both "https://host" and the legacy "https://host/api/v1/memos"
function apiBase(url) {
  return url.replace(/\/+$/, '').replace(/\/api\/v1\/memos$/, '');
}

function apiRequest(method, path, body, callback) {
  var settings = getSettings();
  if (!settings || !settings.url || !settings.token) {
    callback('Not configured');
    return;
  }

  var xhr = new XMLHttpRequest();
  xhr.open(method, apiBase(settings.url) + '/api/v1/' + path, true);
  xhr.setRequestHeader('Content-Type', 'application/json');
  xhr.setRequestHeader('Authorization', 'Bearer ' + settings.token);
  xhr.timeout = TIMEOUT_MS;

  xhr.onload = function() {
    if (xhr.status < 200 || xhr.status >= 300) {
      callback('Server error ' + xhr.status);
      return;
    }
    var data = {};
    try {
      if (xhr.responseText) data = JSON.parse(xhr.responseText);
    } catch (err) {
      callback('Invalid response');
      return;
    }
    callback(null, data);
  };
  xhr.onerror = function() {
    callback('Network error');
  };
  xhr.ontimeout = function() {
    callback('Request timed out');
  };

  xhr.send(body ? JSON.stringify(body) : null);
}

function filterMemos(memos, accept, limit) {
  var result = [];
  for (var i = 0; i < memos.length && result.length < limit; i++) {
    var memo = memos[i];
    // comments are memos with a parent, archived ones are not listed
    if (memo.parent) continue;
    if (memo.state === 'ARCHIVED' || memo.rowStatus === 'ARCHIVED') continue;
    if (accept(memo)) result.push(memo);
  }
  return result;
}

// Collect memos page by page until `limit` memos pass `accept`. Without a
// connection the cached memos are used; callback(err, memos, offline).
function fetchMemos(accept, limit, callback) {
  var fetched = [];

  function fetchPage(pageToken, pagesLeft) {
    var path = 'memos?pageSize=' + PAGE_SIZE +
        (pageToken ? '&pageToken=' + encodeURIComponent(pageToken) : '');

    apiRequest('GET', path, null, function(err, data) {
      if (err) {
        var cached = loadCache();
        if (isOfflineError(err) && cached.length > 0) {
          callback(null, filterMemos(cached, accept, limit), true);
        } else {
          callback(err);
        }
        return;
      }

      fetched = fetched.concat(data.memos || []);
      var result = filterMemos(fetched, accept, limit);
      if (result.length < limit && data.nextPageToken && pagesLeft > 1) {
        fetchPage(data.nextPageToken, pagesLeft - 1);
      } else {
        updateCache(fetched, !data.nextPageToken);
        callback(null, result, false);
      }
    });
  }

  fetchPage(null, MAX_PAGES);
}

function memoItems(memos) {
  return memos.map(function(memo) {
    return {
      id: memo.name,
      title: memoTitle(memo),
      subtitle: memoSubtitle(memo),
      flags: hasComments(memo) ? FLAG_HAS_COMMENTS : 0
    };
  });
}

// Replies with one page of the memos passing `accept`, starting at the
// offset the watch asked for
function replyMemoPage(req, accept) {
  var offset = req.offset || 0;
  // one extra memo tells whether there is another page
  fetchMemos(
      accept, offset + MAX_LIST_ITEMS + 1, function(err, memos, offline) {
        if (err) {
          replyError(req, err);
          return;
        }
        var page = memos.slice(offset, offset + MAX_LIST_ITEMS);
        replyList(
            req, memoItems(page), offline,
            memos.length > offset + MAX_LIST_ITEMS);
      });
}

/* =========================================================
 * Offline queue for new memos
 * ========================================================= */

function loadQueue() {
  try {
    return loadStored(QUEUE_KEY) || [];
  } catch (err) {
    return [];
  }
}

function saveQueue(queue) {
  localStorage.setItem(QUEUE_KEY, JSON.stringify(queue));
}

var flushing = false;

// Sends queued memos one by one; stops at the first network failure
function flushQueue() {
  if (flushing || loadQueue().length === 0) return;
  flushing = true;
  var sent = 0;

  function next() {
    var queue = loadQueue();
    if (queue.length === 0) {
      flushing = false;
      if (sent > 0) {
        notifyWatch(
            sent === 1 ? 'Queued memo sent' : sent + ' queued memos sent',
            true);
        scheduleGlanceUpdate();
      }
      return;
    }
    apiRequest(
        'POST', 'memos', {content: queue[0].content}, function(err, memo) {
          if (err && isOfflineError(err)) {
            flushing = false;
            return;
          }
          // a rejected memo would block the queue forever, so it is dropped
          if (err)
            notifyWatch('Queued memo rejected: ' + err, false);
          else {
            sent++;
            if (memo && memo.name)
              saveCache([slimMemo(memo)].concat(loadCache()));
          }
          saveQueue(loadQueue().slice(1));
          next();
        });
  }
  next();
}

/* =========================================================
 * App glance: open tasks of the memos with the todo tag
 * ========================================================= */

function todoTag() {
  var settings = getSettings() || {};
  return (settings.todoTag || DEFAULT_TODO_TAG).replace(/^#/, '').trim() ||
      DEFAULT_TODO_TAG;
}

function updateGlance() {
  var tag = todoTag();
  fetchMemos(
      function(memo) {
        return hasTag(memo, tag);
      },
      PAGE_SIZE * MAX_PAGES,
      function(err, memos) {
        if (err) return;
        var open = 0;
        memos.forEach(function(memo) {
          findTasks(memo.content || '').forEach(function(task) {
            if (!task.done) open++;
          });
        });
        enqueue({'GLANCE_COUNT': open});
      });
}

var glanceTimer = null;

// Several changes in a row only trigger one update
function scheduleGlanceUpdate(delay) {
  if (glanceTimer) clearTimeout(glanceTimer);
  glanceTimer = setTimeout(function() {
    glanceTimer = null;
    updateGlance();
  }, delay || 2000);
}

/* =========================================================
 * Request handlers
 * ========================================================= */

function handleCreate(req) {
  var settings = getSettings() || {};
  var defaultTags = settings.tags ? settings.tags.split(/\s+/) : DEFAULT_TAGS;
  var content = parseMemo(req.text || '', defaultTags);
  if (content === null) {
    replyError(req, 'Memo is empty');
    return;
  }
  apiRequest('POST', 'memos', {content: content}, function(err, memo) {
    if (err && isOfflineError(err)) {
      // sent later, when the phone is online again
      var queue = loadQueue();
      queue.push({content: content});
      saveQueue(queue);
      replyOk(req, {'TEXT': translate('Offline: memo queued')});
      return;
    }
    if (err) {
      replyError(req, err);
      return;
    }
    // so the new memo can also be viewed offline
    if (memo && memo.name) saveCache([slimMemo(memo)].concat(loadCache()));
    scheduleGlanceUpdate();
    replyOk(req);
  });
}

function handleListRecent(req) {
  replyMemoPage(req, function() {
    return true;
  });
}

function handleListByTag(req) {
  replyMemoPage(req, function(memo) {
    return hasTag(memo, req.target);
  });
}

function handleListPinned(req) {
  replyMemoPage(req, function(memo) {
    return !!memo.pinned;
  });
}

// All words of the query must appear in the memo text or its tags
function handleSearch(req) {
  var words = (req.text || '').toLowerCase().split(/\s+/).filter(function(w) {
    return w !== '';
  });
  if (words.length === 0) {
    replyError(req, 'Nothing to search');
    return;
  }
  replyMemoPage(req, function(memo) {
    var text =
        ((memo.content || '') + ' ' + memoTags(memo).join(' ')).toLowerCase();
    return words.every(function(word) {
      return text.indexOf(word) !== -1;
    });
  });
}

function handleListTags(req) {
  // there is no stable tag endpoint across Memos versions, so count
  // the tags of the most recent memos instead
  fetchMemos(
      function() {
        return true;
      },
      PAGE_SIZE * MAX_PAGES,
      function(err, memos, offline) {
        if (err) {
          replyError(req, err);
          return;
        }
        var counts = {};
        memos.forEach(function(memo) {
          memoTags(memo).forEach(function(tag) {
            counts[tag] = (counts[tag] || 0) + 1;
          });
        });
        var tags = Object.keys(counts)
                       .sort(function(a, b) {
                         return counts[b] - counts[a] || a.localeCompare(b);
                       })
                       .slice(0, MAX_TAG_ITEMS);

        replyList(
            req, tags.map(function(tag) {
              return {
                id: tag,
                title: truncateUtf8('#' + tag, MAX_TITLE_BYTES),
                subtitle: translate(
                    counts[tag] + (counts[tag] === 1 ? ' memo' : ' memos'))
              };
            }),
            offline);
      });
}

// Comments oldest first, each with its date, separated by rules
function commentLines(comments) {
  var lines = [];
  comments.slice()
      .sort(function(a, b) {
        return memoTime(a) - memoTime(b);
      })
      .forEach(function(comment, index) {
        if (index > 0) lines.push('-');
        lines.push('m' + formatDate(comment.displayTime || comment.createTime));
        lines = lines.concat(markdownLines(comment.content || ''));
      });
  return lines;
}

function handleGet(req) {
  apiRequest('GET', req.target, null, function(err, memo) {
    if (err && isOfflineError(err)) memo = findInCache(req.target);
    if (!memo) {
      replyError(req, err || 'Memo not found');
      return;
    }
    // the flag decides whether the watch offers "Comments"
    replyOk(req, {
      'TEXT': markdownToLines(memo.content || ''),
      'ITEM_FLAGS': hasComments(memo) ? FLAG_HAS_COMMENTS : 0
    });
  });
}

function handleListComments(req) {
  apiRequest('GET', req.target + '/comments', null, function(err, data) {
    if (err) {
      replyError(req, err);
      return;
    }
    var comments = data.memos || [];
    replyOk(req, {
      'TEXT': comments.length > 0 ? joinLines(commentLines(comments)) :
                                    'p' + translate('No comments yet')
    });
  });
}

function handleArchive(req) {
  apiRequest(
      'PATCH', req.target + '?updateMask=state', {state: 'ARCHIVED'},
      function(err) {
        if (!err) {
          removeFromCache(req.target);
          replyOk(req);
          return;
        }
        // Memos versions before 0.22 call the field rowStatus
        apiRequest(
            'PATCH', req.target + '?updateMask=row_status',
            {rowStatus: 'ARCHIVED'}, function(legacyErr) {
              if (legacyErr) {
                replyError(req, err);
                return;
              }
              removeFromCache(req.target);
              replyOk(req);
            });
      });
}

function handleDelete(req) {
  apiRequest('DELETE', req.target, null, function(err) {
    if (err) {
      replyError(req, err);
      return;
    }
    removeFromCache(req.target);
    replyOk(req);
  });
}

function handleComment(req) {
  var content = applyKeywords(req.text || '').trim();
  if (isBlankMarkdown(content)) {
    replyError(req, 'Comment is empty');
    return;
  }
  apiRequest(
      'POST', req.target + '/comments', {content: content}, function(err) {
        if (err)
          replyError(req, err);
        else
          replyOk(req);
      });
}

// Tasks of one memo, in document order. Task ids are
// "<memo name>#<task index within the memo>".
function handleListTasks(req) {
  apiRequest('GET', req.target, null, function(err, memo) {
    var offline = false;
    if (err && isOfflineError(err)) {
      memo = findInCache(req.target);
      offline = !!memo;
    }
    if (!memo) {
      replyError(req, err || 'Memo not found');
      return;
    }
    var tasks = findTasks(memo.content || '').slice(0, MAX_TASK_ITEMS);
    replyList(
        req, tasks.map(function(task, index) {
          return {
            id: memo.name + '#' + index,
            title: task.text,
            subtitle: '',
            flags: task.done ? 1 : 0
          };
        }),
        offline);
  });
}

function handleToggleTask(req) {
  var separator = (req.target || '').lastIndexOf('#');
  var name = req.target.substring(0, separator);
  var index = parseInt(req.target.substring(separator + 1), 10);

  // always toggle the current content, so other edits are not lost
  apiRequest('GET', name, null, function(err, memo) {
    if (err) {
      replyError(req, err);
      return;
    }
    var task = findTasks(memo.content || '')[index];
    // the watch sends the task text it shows, to catch edits since then
    if (!task || (req.text && task.text !== req.text)) {
      replyError(req, 'Memo changed, reload tasks');
      return;
    }
    var content = toggleTaskLine(memo.content, task.line);
    apiRequest(
        'PATCH', name + '?updateMask=content', {content: content},
        function(patchErr) {
          if (patchErr) {
            replyError(req, patchErr);
            return;
          }
          memo.content = content;
          replaceInCache(memo);
          scheduleGlanceUpdate();
          replyOk(req, {'ITEM_FLAGS': task.done ? 0 : 1});
        });
  });
}

function handleAppend(req) {
  var text = applyKeywords(req.text || '').trim();
  if (isBlankMarkdown(text)) {
    replyError(req, 'Nothing to append');
    return;
  }
  // always append to the current content, so other edits are not lost
  apiRequest('GET', req.target, null, function(err, memo) {
    if (err) {
      replyError(req, err);
      return;
    }
    var content = appendToContent(memo.content || '', text);
    apiRequest(
        'PATCH', req.target + '?updateMask=content', {content: content},
        function(patchErr) {
          if (patchErr) {
            replyError(req, patchErr);
            return;
          }
          memo.content = content;
          replaceInCache(memo);
          scheduleGlanceUpdate();
          replyOk(req);
        });
  });
}

function handleTogglePin(req) {
  apiRequest('GET', req.target, null, function(err, memo) {
    if (err) {
      replyError(req, err);
      return;
    }
    var pinned = !memo.pinned;
    apiRequest(
        'PATCH', req.target + '?updateMask=pinned', {pinned: pinned},
        function(patchErr) {
          if (patchErr) {
            replyError(req, patchErr);
            return;
          }
          memo.pinned = pinned;
          replaceInCache(memo);
          replyOk(req, {
            'ITEM_FLAGS': pinned ? 1 : 0,
            'TEXT': translate(pinned ? 'Pinned' : 'Unpinned')
          });
        });
  });
}

var HANDLERS = {};
HANDLERS[ACTION_CREATE] = handleCreate;
HANDLERS[ACTION_LIST_RECENT] = handleListRecent;
HANDLERS[ACTION_LIST_TAGS] = handleListTags;
HANDLERS[ACTION_LIST_BY_TAG] = handleListByTag;
HANDLERS[ACTION_GET] = handleGet;
HANDLERS[ACTION_ARCHIVE] = handleArchive;
HANDLERS[ACTION_DELETE] = handleDelete;
HANDLERS[ACTION_COMMENT] = handleComment;
HANDLERS[ACTION_LIST_TASKS] = handleListTasks;
HANDLERS[ACTION_TOGGLE_TASK] = handleToggleTask;
HANDLERS[ACTION_APPEND] = handleAppend;
HANDLERS[ACTION_SEARCH] = handleSearch;
HANDLERS[ACTION_LIST_PINNED] = handleListPinned;
HANDLERS[ACTION_TOGGLE_PIN] = handleTogglePin;
HANDLERS[ACTION_LIST_COMMENTS] = handleListComments;

/* =========================================================
 * AppMessage handler
 * ========================================================= */
Pebble.addEventListener('appmessage', function(e) {
  var payload = e.payload || {};
  var req = {
    requestId: payload['REQUEST_ID'],
    action: payload['ACTION'],
    target: payload['ITEM_ID'],  // memo name or tag the request refers to
    text: payload['TEXT'],
    offset: payload['ITEM_INDEX']  // first item of the requested page
  };

  var handler = HANDLERS[req.action];
  if (!handler) {
    console.log('Ostrakon: Unknown action', req.action);
    return;
  }
  console.log('Ostrakon: Request', req.action, req.target || '');

  // any request is a chance to send memos queued while offline
  flushQueue();

  try {
    handler(req);
  } catch (err) {
    console.log('Ostrakon: Handler error', err);
    replyError(req, 'Phone error');
  }
});

/* =========================================================
 * Lifecycle
 * ========================================================= */
// The watch keeps its own copy of settings it needs without the phone
function sendWatchSettings() {
  var settings = getSettings() || {};
  enqueue({
    'VIBRATE': settings.vibrate === false ? 0 : 1,
    'LANGUAGE': i18n.LANGUAGE_CODES[i18n.settingValue(settings.language)]
  });
}

// Tells the watch right away if the server or token do not work
function testConnection() {
  apiRequest('GET', 'memos?pageSize=1', null, function(err) {
    if (!err)
      notifyWatch('Connected to Memos', true);
    else if (err === 'Server error 401' || err === 'Server error 403') {
      notifyWatch('Token rejected (' + err.slice(-3) + ')', false);
    } else if (err === 'Server error 404') {
      notifyWatch('API not found, check URL', false);
    } else if (isOfflineError(err)) {
      notifyWatch('Server not reachable', false);
    } else {
      notifyWatch(err, false);
    }
  });
}

Pebble.addEventListener('ready', function() {
  console.log('Ostrakon: PebbleKit JS ready');
  sendWatchSettings();
  flushQueue();
  scheduleGlanceUpdate(GLANCE_DELAY_MS);
});

/* ---------- Settings page (Clay) ---------- */
// The settings stay in SETTINGS_KEY; Clay is only the form in front of
// them, filled from them every time it opens. It is built anew each time,
// in the language that is current then.
var clay = null;

function createClay() {
  var Clay = require('./vendor/clay');
  var strings = i18n.pageStrings(currentLanguage());
  clay = new Clay(
      require('./clay-config')(strings), require('./clay-custom'),
      {autoHandleEvents: false});
  // Clay only fills meta on "ready", which it does not handle here
  clay.meta.userData = {strings: strings};
  return clay;
}

function clayValue(settings, key) {
  var item = settings[key];
  return item !== null && typeof item === 'object' ? item.value : item;
}

Pebble.addEventListener('showConfiguration', function() {
  console.log('Ostrakon: Opening config page');
  var current = getSettings() || {};
  createClay().setSettings({
    'SETTING_URL': current.url || '',
    'SETTING_TOKEN': current.token || '',
    'SETTING_TAGS': current.tags !== undefined ? current.tags :
                                                 DEFAULT_TAGS.join(' '),
    'SETTING_TODO_TAG': current.todoTag || '#' + DEFAULT_TODO_TAG,
    'VIBRATE': current.vibrate !== false,
    'SETTING_LANGUAGE': i18n.settingValue(current.language)
  });
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e.response) return;

  try {
    var values = (clay || createClay()).getSettings(e.response, false);
    var settings = {
      url: String(clayValue(values, 'SETTING_URL') || '').trim(),
      token: String(clayValue(values, 'SETTING_TOKEN') || '').trim(),
      tags: String(clayValue(values, 'SETTING_TAGS') || '').trim(),
      todoTag: String(clayValue(values, 'SETTING_TODO_TAG') || '').trim(),
      vibrate: !!clayValue(values, 'VIBRATE'),
      language: i18n.settingValue(clayValue(values, 'SETTING_LANGUAGE'))
    };
    localStorage.setItem(SETTINGS_KEY, JSON.stringify(settings));
    console.log('Ostrakon: Settings saved');
    sendWatchSettings();
    testConnection();
    scheduleGlanceUpdate();
  } catch (err) {
    console.log('Ostrakon: Failed to parse settings', err);
  }
});
