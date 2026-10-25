// Ostrakon homepage. Plain JS, no build step, no dependencies.
(function() {
  'use strict';

  var REPO = 'minils/ostrakon';
  var REPO_URL = 'https://github.com/' + REPO;
  var RELEASES_URL = REPO_URL + '/releases';
  var reduceMotion = window.matchMedia &&
      window.matchMedia('(prefers-reduced-motion: reduce)').matches;

  function $(sel, root) { return (root || document).querySelector(sel); }
  function $$(sel, root) {
    return Array.prototype.slice.call((root || document).querySelectorAll(sel));
  }
  function escapeHtml(s) {
    return s.replace(/[&<>"]/g, function(c) {
      return {'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;'}[c];
    });
  }
  function store(key, value) {
    try {
      if (value === undefined) return localStorage.getItem(key);
      if (value === null) localStorage.removeItem(key);
      else localStorage.setItem(key, value);
    } catch (e) {}
    return null;
  }

  /* =========================================================
   * The shard: the app icon, in the style of the PebbleOS icons
   * (a port of store/make_art.py), and made-up glyphs for the
   * scratched writing behind the hero
   * ========================================================= */

  var SVG = 'http://www.w3.org/2000/svg';
  var SHARD = [[1.5, 7.5], [5.5, 3.5], [14.5, 3.5], [16.5, 6], [19, 5], [19.5, 7.5],
               [23, 9], [23, 15], [18, 20.5], [6.5, 20.5], [1.5, 15.5]];
  var WRITING = [[[5.5, 8], [14, 8]], [[5.5, 11.5], [9, 11.5]], [[11, 11.5], [19, 11.5]],
                 [[5.5, 15], [12, 15]], [[14, 15], [18, 15]]];

  function rng(seed) {  // mulberry32
    return function() {
      seed |= 0; seed = seed + 0x6D2B79F5 | 0;
      var t = Math.imul(seed ^ seed >>> 15, 1 | seed);
      t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t;
      return ((t ^ t >>> 14) >>> 0) / 4294967296;
    };
  }
  function uniform(r, a, b) { return a + (b - a) * r(); }
  function f(n) { return Math.round(n * 100) / 100; }

  // One glyph of 2-3 strokes, hooks, arcs or crossbars, as SVG path data
  function glyphPaths(r, x, y, w, h) {
    var paths = [];
    var n = 2 + Math.floor(r() * 2);
    for (var i = 0; i < n; i++) {
      var kind = r(), x0, mid, yy;
      if (kind < 0.35) {
        x0 = x + uniform(r, 0, w * 0.6);
        paths.push('M' + f(x0) + ' ' + f(y + h) + 'L' + f(x0 + uniform(r, -0.4, 0.6) * w) + ' ' + f(y));
      } else if (kind < 0.6) {
        x0 = x + uniform(r, 0, w * 0.5);
        mid = y + uniform(r, 0.5, 1.0) * h;
        paths.push('M' + f(x0) + ' ' + f(y) + 'L' + f(x0) + ' ' + f(mid) + 'L' +
                   f(x0 + w * 0.5) + ' ' + f(mid - uniform(r, 0, 0.4) * h));
      } else if (kind < 0.85) {
        var top = y + uniform(r, 0, 0.4) * h;
        var cx = x + w / 2, cy = (top + y + h) / 2, rx = w / 2, ry = (y + h - top) / 2;
        var a0 = [0, 90, 180, 270][Math.floor(r() * 4)] * Math.PI / 180;
        var sweep = (120 + Math.floor(r() * 101)) * Math.PI / 180;
        var a1 = a0 + sweep;
        paths.push('M' + f(cx + rx * Math.cos(a0)) + ' ' + f(cy + ry * Math.sin(a0)) +
                   'A' + f(rx) + ' ' + f(ry) + ' 0 ' + (sweep > Math.PI ? 1 : 0) + ' 1 ' +
                   f(cx + rx * Math.cos(a1)) + ' ' + f(cy + ry * Math.sin(a1)));
      } else {
        yy = y + uniform(r, 0.2, 0.8) * h;
        paths.push('M' + f(x) + ' ' + f(yy) + 'L' + f(x + w) + ' ' + f(yy + uniform(r, -0.2, 0.2) * h));
      }
    }
    return paths;
  }

  // Thicker lines when small, like the 48px icon. When animated, the
  // writing is scratched on stroke by stroke.
  function drawShard(svg) {
    var small = svg.classList.contains('shard-sm');
    var animate = svg.hasAttribute('data-animate');
    var pts = SHARD.map(function(p) { return p.join(','); }).join(' ');
    var ink = WRITING.map(function(line, i) {
      var style = animate ? ' style="animation-delay:' + (0.35 + i * 0.22).toFixed(2) + 's"' : '';
      return '<path d="M' + line[0].join(' ') + 'L' + line[1].join(' ') + '" pathLength="1"' + style + '/>';
    }).join('');
    svg.innerHTML =
        '<polygon class="body" points="' + pts + '" stroke-width="' + (small ? 1.8 : 1.4) + '"/>' +
        '<g class="ink" stroke-width="' + (small ? 1.3 : 1) + '">' + ink + '</g>';
  }
  $$('svg[data-shard]').forEach(drawShard);

  // Faint scratched writing behind the hero
  (function glyphField() {
    var field = $('.glyph-field');
    if (!field) return;
    var r = rng(2026);
    var d = [];
    for (var row = 0; row < 14; row++) {
      var x = uniform(r, -4, 4);
      while (x < 200) {
        var len = 2 + Math.floor(r() * 5);
        for (var i = 0; i < len; i++) {
          d = d.concat(glyphPaths(r, x, row * 9 + 3, 2.2, 3.4));
          x += 2.8;
        }
        x += uniform(r, 3, 8);
      }
    }
    field.innerHTML = '<svg viewBox="0 0 200 126" preserveAspectRatio="xMidYMid slice"><g transform="rotate(-6 100 63)"><path d="' +
        d.join('') + '"/></g></svg>';
  })();

  /* =========================================================
   * Theme: auto → light → dark
   * ========================================================= */

  var themes = ['auto', 'light', 'dark'];
  function applyTheme(theme) {
    if (theme === 'auto') document.documentElement.removeAttribute('data-theme');
    else document.documentElement.setAttribute('data-theme', theme);
    $('.theme-label').textContent = theme;
  }
  var theme = store('ostrakon-theme') || 'auto';
  if (themes.indexOf(theme) < 0) theme = 'auto';
  applyTheme(theme);
  $('#theme-toggle').addEventListener('click', function() {
    theme = themes[(themes.indexOf(theme) + 1) % themes.length];
    applyTheme(theme);
    store('ostrakon-theme', theme === 'auto' ? null : theme);
  });

  /* =========================================================
   * GitHub: stars and the latest release
   * ========================================================= */

  var release = null;  // {tag, date, url, pbw}

  function getJson(url) {
    var cacheKey = 'gh:' + url;
    try {
      var cached = JSON.parse(sessionStorage.getItem(cacheKey) || 'null');
      if (cached) return Promise.resolve(cached);
    } catch (e) {}
    return fetch(url, {headers: {Accept: 'application/vnd.github+json'}})
        .then(function(res) {
          if (!res.ok) throw new Error('HTTP ' + res.status);
          return res.json();
        })
        .then(function(data) {
          try { sessionStorage.setItem(cacheKey, JSON.stringify(data)); } catch (e) {}
          return data;
        });
  }

  if (window.fetch && window.Promise) {
    getJson('https://api.github.com/repos/' + REPO).then(function(repo) {
      if (repo && typeof repo.stargazers_count === 'number') {
        var el = $('#gh-stars');
        el.textContent = repo.stargazers_count;
        el.hidden = false;
      }
    }).catch(function() {});

    // the list rather than /releases/latest, which answers 404 while there is none
    getJson('https://api.github.com/repos/' + REPO + '/releases?per_page=10').then(function(list) {
      var line = $('#release-line');
      var text = $('#release-text');
      var published = (Array.isArray(list) ? list : []).filter(function(r) { return !r.draft; });
      var data = published.filter(function(r) { return !r.prerelease; })[0] || published[0];
      if (!data || !data.tag_name) {
        line.classList.add('none');
        text.innerHTML = 'no release tagged yet · <a href="' + REPO_URL +
            '/actions">grab a CI build</a> or watch <a href="' + RELEASES_URL + '">releases</a>';
        return;
      }
      var pbw = (data.assets || []).filter(function(a) { return /\.pbw$/i.test(a.name); })[0];
      release = {
        tag: data.tag_name,
        date: (data.published_at || '').slice(0, 10),
        url: data.html_url || RELEASES_URL,
        pbw: pbw ? pbw.browser_download_url : null
      };
      line.classList.add('ok');
      text.innerHTML = 'latest: <a href="' + escapeHtml(release.url) + '">' + escapeHtml(release.tag) +
          '</a> · ' + escapeHtml(release.date) + ' · <a href="' + RELEASES_URL + '">all releases</a>';
      var btn = $('#dl-btn');
      btn.href = release.pbw || release.url;
      $('#dl-label').textContent = release.pbw ? 'Download ostrakon.pbw' : 'Get ' + release.tag;
    }).catch(function() {
      $('#release-text').innerHTML = 'see <a href="' + RELEASES_URL + '">all releases on GitHub</a>';
    });
  }

  /* =========================================================
   * Spoken keywords: the parser from src/pkjs/index.js
   * ========================================================= */

  var DEFAULT_KEYWORDS = {
    en: [
      {word: 'check box', markdown: '- [ ]'},
      {word: 'checkbox', markdown: '- [ ]'}, {word: 'new line', markdown: '\\n'},
      {word: 'title big', markdown: '#'}, {word: 'title medium', markdown: '##'},
      {word: 'title small', markdown: '###'}
    ],
    de: [
      {word: 'Checkbox', markdown: '- [ ]'},
      {word: 'Kontrollkästchen', markdown: '- [ ]'},
      {word: 'neue Zeile', markdown: '\\n'},
      {word: 'Überschrift groß', markdown: '#'},
      {word: 'Titel groß', markdown: '#'},
      {word: 'Überschrift mittel', markdown: '##'},
      {word: 'Titel mittel', markdown: '##'},
      {word: 'Überschrift klein', markdown: '###'},
      {word: 'Titel klein', markdown: '###'}
    ],
    fr: [
      {word: 'case à cocher', markdown: '- [ ]'},
      {word: 'nouvelle ligne', markdown: '\\n'},
      {word: 'grand titre', markdown: '#'}, {word: 'titre grand', markdown: '#'},
      {word: 'titre moyen', markdown: '##'},
      {word: 'petit titre', markdown: '###'},
      {word: 'titre petit', markdown: '###'}
    ],
    es: [
      {word: 'casilla de verificación', markdown: '- [ ]'},
      {word: 'nueva línea', markdown: '\\n'},
      {word: 'título grande', markdown: '#'},
      {word: 'título mediano', markdown: '##'},
      {word: 'título pequeño', markdown: '###'}
    ],
    it: [
      {word: 'casella di controllo', markdown: '- [ ]'},
      {word: 'nuova riga', markdown: '\\n'},
      {word: 'titolo grande', markdown: '#'},
      {word: 'titolo medio', markdown: '##'},
      {word: 'titolo piccolo', markdown: '###'}
    ]
  };

  var PATTERNS = (function keywordPatterns() {
    var keywords = Object.keys(DEFAULT_KEYWORDS).reduce(function(all, language) {
      return all.concat(DEFAULT_KEYWORDS[language]);
    }, []);
    return keywords.slice()
        .sort(function(a, b) { return b.word.length - a.word.length; })
        .map(function(keyword) {
          var markdown = keyword.markdown.replace(/\\n/g, '\n');
          if (markdown.slice(-1) !== '\n') markdown += ' ';
          var words = keyword.word.split(/\s+/).map(function(word) {
            return word.replace(/[.*+?^${}()|[\]\\\/]/g, '\\$&');
          });
          return {
            regex: new RegExp('(?:' + words.join('\\s+') + ')(?=[\\s,.!?;:]|$)[ \\t,.]*', 'gi'),
            markdown: markdown
          };
        });
  })();

  function applyKeywords(text) {
    var content = PATTERNS.reduce(function(result, pattern) {
      return result.replace(pattern.regex, function(phrase, offset, all) {
        var before = offset > 0 ? all.charAt(offset - 1) : ' ';
        return /[\s,.!?;:]/.test(before) ? pattern.markdown : phrase;
      });
    }, text);
    return content.replace(/[ \t]+\n/g, '\n');
  }

  function takeSpokenTags(text) {
    var tags = [];
    var rest = text.replace(/hashtag\s+([^\s,.!?;:]+)/gi, function(phrase, word) {
      tags.push('#' + word.toLowerCase());
      return '';
    });
    return {tags: tags, text: rest.trim()};
  }

  function isBlankMarkdown(text) {
    return text.replace(/[-#>`\[\]\s]/g, '') === '';
  }

  function parseMemo(text, defaultTags) {
    var spoken = takeSpokenTags(applyKeywords(text));
    if (isBlankMarkdown(spoken.text)) return null;
    return spoken.text + '\n\n' + defaultTags.concat(spoken.tags).join(' ');
  }

  function highlightMarkdown(md) {
    return md.split('\n').map(function(line) {
      var html = escapeHtml(line);
      if (/^#{1,3} /.test(line)) return '<span class="m-h">' + html + '</span>';
      html = html.replace(/^(- \[[ x]\])/, '<span class="m-t">$1</span>');
      return html.replace(/(^|\s)(#[^\s#]+)/g, '$1<span class="m-tag">$2</span>');
    }).join('\n');
  }

  // A rough idea of how memo_view.c draws the memo
  function renderPebble(md) {
    if (md === null) return '<div class="pbl-empty">Nothing to send yet.</div>';
    return md.split('\n').map(function(line) {
      var m;
      var tagged = function(s) {
        return escapeHtml(s).replace(/(^|\s)(#[^\s#]+)/g, '$1<span class="pbl-tag">$2</span>');
      };
      if ((m = /^(#{1,3}) +(.*)$/.exec(line))) {
        return '<div class="pbl-h' + m[1].length + '">' + escapeHtml(m[2]) + '</div>';
      }
      if ((m = /^\s*[-*] \[([ xX])\] ?(.*)$/.exec(line))) {
        return '<div class="pbl-task' + (m[1] === ' ' ? '' : ' done') + '"><span>' + tagged(m[2]) + '</span></div>';
      }
      if ((m = /^\s*[-*] (.*)$/.exec(line))) return '<div class="pbl-bullet"><span>' + tagged(m[1]) + '</span></div>';
      if ((m = /^> ?(.*)$/.exec(line))) return '<div class="pbl-quote">' + tagged(m[1]) + '</div>';
      if (!line.trim()) return '';
      return '<div>' + tagged(line) + '</div>';
    }).join('');
  }

  var PRESETS = {
    en: 'title big Weekend in the Alps new line checkbox book train tickets new line checkbox pack hiking boots new line checkbox download offline maps hashtag travel hashtag todo',
    de: 'Überschrift groß Einkaufen neue Zeile Checkbox Olivenöl neue Zeile Checkbox frisches Basilikum neue Zeile Checkbox Parmesan Hashtag Küche',
    fr: 'grand titre Idées pour samedi nouvelle ligne case à cocher visiter le marché nouvelle ligne case à cocher appeler Claire hashtag perso',
    es: 'título grande Lista de la compra nueva línea casilla de verificación tomates nueva línea casilla de verificación pan integral hashtag casa',
    it: 'titolo grande Cena di venerdì nuova riga casella di controllo comprare il vino nuova riga casella di controllo prenotare il tavolo hashtag amici'
  };

  var spoken = $('#spoken');
  var defaultTagsInput = $('#default-tags');
  function updatePlayground() {
    var tags = defaultTagsInput.value.split(/[\s,]+/).filter(Boolean).map(function(t) {
      return t.charAt(0) === '#' ? t : '#' + t;
    });
    var md = parseMemo(spoken.value, tags);
    $('#md-out').innerHTML = md === null ? '<span class="t-c">null  // nothing but keywords: not sent</span>' :
        highlightMarkdown(md);
    $('#pbl-body').innerHTML = renderPebble(md);
  }
  function selectPreset(lang) {
    $$('[data-preset]').forEach(function(b) {
      b.setAttribute('aria-pressed', String(b.getAttribute('data-preset') === lang));
    });
  }

  var dictating = null;
  function stopDictation() {
    if (!dictating) return;
    clearTimeout(dictating);
    dictating = null;
    $('#dictate').classList.remove('rec');
    $('#dictate').textContent = '◉ dictate';
  }
  // Fills in the text word by word, like the watch's dictation does
  function dictate(text) {
    stopDictation();
    var words = text.split(' ');
    var i = 0;
    spoken.value = '';
    $('#dictate').classList.add('rec');
    $('#dictate').textContent = '■ stop';
    (function next() {
      spoken.value += (i ? ' ' : '') + words[i++];
      updatePlayground();
      if (i < words.length) dictating = setTimeout(next, reduceMotion ? 0 : 140 + Math.random() * 120);
      else stopDictation();
    })();
  }

  $$('[data-preset]').forEach(function(btn) {
    btn.addEventListener('click', function() {
      stopDictation();
      var lang = btn.getAttribute('data-preset');
      selectPreset(lang);
      spoken.value = PRESETS[lang];
      updatePlayground();
    });
  });
  $('#dictate').addEventListener('click', function() {
    if (dictating) return stopDictation();
    var active = $('[data-preset][aria-pressed="true"]');
    dictate(PRESETS[active ? active.getAttribute('data-preset') : 'en']);
  });
  spoken.addEventListener('input', function() { stopDictation(); selectPreset(null); updatePlayground(); });
  defaultTagsInput.addEventListener('input', updatePlayground);

  var browserLang = (navigator.language || 'en').slice(0, 2);
  var startLang = PRESETS[browserLang] ? browserLang : 'en';
  selectPreset(startLang);
  spoken.value = PRESETS[startLang];
  updatePlayground();

  /* =========================================================
   * Hero: terminal and the watch
   * ========================================================= */

  (function heroTerminal() {
    var el = $('#hero-term');
    var said = 'title big groceries new line checkbox olive oil new line checkbox basil hashtag kitchen';
    var md = parseMemo(said, ['#pebble']);
    var script = [
      ['cmd', 'pebble install --phone 192.168.1.42 ostrakon.pbw'],
      ['out', '<span class="t-c">Installing app...</span>'],
      ['out', 'App install succeeded.'],
      ['cmd', '# hold the button and say:'],
      ['out', '<span class="t-o">“' + said + '”</span>'],
      ['cmd', '# what lands in your Memos:'],
      ['out', '<span class="t-c">POST /api/v1/memos → 200 OK</span>'],
      ['out', highlightMarkdown(md).replace(/class="m-h"/g, 'class="t-o"')
          .replace(/class="m-t"/g, 'class="t-p"').replace(/class="m-tag"/g, 'class="t-d"')]
    ];
    var html = '';
    var prompt = '<span class="t-p">~ $</span> ';
    function render(extra) { el.innerHTML = html + (extra || '') + '<span class="cursor"></span>'; }
    if (reduceMotion) {
      script.forEach(function(s) { html += (s[0] === 'cmd' ? prompt + escapeHtml(s[1]) : s[1]) + '\n'; });
      html += prompt;
      return render();
    }
    var step = 0;
    function next() {
      if (step >= script.length) { html += prompt; return render(); }
      var s = script[step++];
      if (s[0] === 'out') {
        html += s[1] + '\n';
        render();
        return setTimeout(next, 260);
      }
      var i = 0;
      (function type() {
        render(prompt + escapeHtml(s[1].slice(0, i)));
        if (i++ < s[1].length) return setTimeout(type, 22 + Math.random() * 40);
        html += prompt + escapeHtml(s[1]) + '\n';
        setTimeout(next, 380);
      })();
    }
    setTimeout(next, 600);
  })();

  var SCREENS = [
    ['recent', 'The latest memos'],
    ['memo', 'A memo with heading, tasks and tags'],
    ['tasks', 'Ticking off tasks'],
    ['comments', 'Comments with author and time'],
    ['comment', 'A single comment'],
    ['tags', 'Tags with their number of memos'],
    ['glance', 'Open to-dos in the launcher']
  ];
  function shotSrc(platform, i) {
    return 'img/screens/' + platform + '-' + (i + 1) + '-' + SCREENS[i][0] + '.png';
  }

  // preload, so the buttons feel instant
  SCREENS.forEach(function(s, i) { new Image().src = shotSrc('emery', i); });

  var heroIndex = 0;
  var heroTimer = null;
  function showHero(i) {
    heroIndex = (i + SCREENS.length) % SCREENS.length;
    var img = $('#hero-shot');
    img.src = shotSrc('emery', heroIndex);
    img.alt = 'Ostrakon on a Pebble Time 2: ' + SCREENS[heroIndex][1].toLowerCase();
    img.classList.remove('swap');
    void img.offsetWidth;
    img.classList.add('swap');
    $('#hero-caption').textContent = (heroIndex + 1) + '/' + SCREENS.length + ' ' + SCREENS[heroIndex][0];
  }
  function autoplay(on) {
    clearInterval(heroTimer);
    if (on && !reduceMotion) heroTimer = setInterval(function() { showHero(heroIndex + 1); }, 3200);
  }
  $$('#hero-watch .wbtn').forEach(function(btn) {
    btn.addEventListener('click', function() {
      autoplay(false);
      var act = btn.getAttribute('data-act');
      showHero(act === 'back' ? 0 : act === 'up' ? heroIndex - 1 : heroIndex + 1);
    });
  });
  showHero(0);
  autoplay(true);

  /* =========================================================
   * Screens gallery
   * ========================================================= */

  function showPlatform(platform) {
    $$('.tab').forEach(function(t) {
      t.setAttribute('aria-selected', String(t.getAttribute('data-plat') === platform));
    });
    var size = {emery: [200, 228], basalt: [144, 168], chalk: [180, 180]}[platform];
    $('#shots').innerHTML = SCREENS.map(function(s, i) {
      return '<figure class="shot' + (platform === 'chalk' ? ' round' : '') + '">' +
          '<span class="shot-frame"><img src="' + shotSrc(platform, i) + '" width="' + size[0] +
          '" height="' + size[1] + '" loading="lazy" alt="' + platform + ': ' + s[1] + '"></span>' +
          '<figcaption><b>' + (i + 1) + '.</b> ' + s[1] + '</figcaption></figure>';
    }).join('');
  }
  $$('.tab').forEach(function(t) {
    t.addEventListener('click', function() { showPlatform(t.getAttribute('data-plat')); });
  });
  showPlatform('emery');

  /* =========================================================
   * Copy buttons
   * ========================================================= */

  $$('[data-copy]').forEach(function(btn) {
    btn.addEventListener('click', function() {
      var text = $('#' + btn.getAttribute('data-copy')).textContent
          .split('\n').map(function(l) { return l.replace(/^\$ /, '').replace(/\s+#.*$/, ''); })
          .join('\n');
      var done = function() {
        btn.textContent = 'copied ✓';
        setTimeout(function() { btn.textContent = 'copy'; }, 1500);
      };
      if (navigator.clipboard) navigator.clipboard.writeText(text).then(done, function() {});
    });
  });

  /* =========================================================
   * The shell (press `)
   * ========================================================= */

  var shell = $('#shell');
  var out = $('#shell-out');
  var input = $('#shell-in');
  var history = [];
  var histPos = 0;
  var lastFocus = null;

  function print(html) {
    out.insertAdjacentHTML('beforeend', html + '\n');
    out.scrollTop = out.scrollHeight;
  }
  function link(url, label) {
    return '<a href="' + url + '" target="_blank" rel="noopener">' + (label || url) + '</a>';
  }

  var COMMANDS = {
    help: function() {
      return 'commands:\n' + [
        ['about', 'what is this'], ['features', 'what it does'],
        ['release', 'latest release'], ['github', 'open the repository'],
        ['releases', 'open the releases page'], ['platforms', 'supported watches'],
        ['dictate <text>', 'run the keyword parser'], ['cat package.json', ''],
        ['theme [auto|light|dark]', ''], ['clear', ''], ['exit', '']
      ].map(function(c) {
        return '  <span class="t-p">' + escapeHtml(c[0]) + '</span>' +
            (c[1] ? '<span class="t-c">' + Array(26 - c[0].length).join(' ') + c[1] + '</span>' : '');
      }).join('\n');
    },
    about: function() {
      return 'Ostrakon: a Pebble watch app for your self-hosted Memos server.\n' +
          'Dictate memos, browse and search them, tick off tasks, add comments.\n' +
          'Watch side in C, phone side in PebbleKit JS. MIT licensed.';
    },
    features: function() {
      return '<span class="t-o">write</span>     dictation · spoken keywords · drafts · offline queue · append · comment\n' +
          '<span class="t-o">read</span>      recent · pinned · tags · voice search · markdown · paging · cache\n' +
          '<span class="t-o">organize</span>  tasks · pin · archive · delete · app glance with open to-dos';
    },
    release: function() {
      if (release) {
        return release.tag + ' (' + release.date + ')\n' + link(release.pbw || release.url);
      }
      return '<span class="t-c">no release found yet.</span> try: ' + link(RELEASES_URL);
    },
    github: function() { window.open(REPO_URL, '_blank', 'noopener'); return 'opening ' + link(REPO_URL); },
    releases: function() {
      window.open(RELEASES_URL, '_blank', 'noopener');
      return 'opening ' + link(RELEASES_URL);
    },
    platforms: function() {
      return ['basalt   Pebble Time, Time Steel', 'chalk    Pebble Time Round', 'diorite  Pebble 2',
              'emery    Pebble Time 2', 'flint    Pebble 2 Duo', 'gabbro   Pebble Round 2'].join('\n');
    },
    dictate: function(args) {
      if (!args) return 'usage: dictate checkbox buy milk hashtag shopping';
      var md = parseMemo(args, []);
      return md === null ? '<span class="t-c">null (nothing to send)</span>' :
          highlightMarkdown(md).replace(/class="m-h"/g, 'class="t-o"')
              .replace(/class="m-t"/g, 'class="t-p"').replace(/class="m-tag"/g, 'class="t-d"');
    },
    cat: function(args) {
      if (args === 'package.json') {
        return escapeHtml(JSON.stringify({
          name: 'ostrakon', author: 'minils',
          pebble: {displayName: 'Ostrakon', uuid: '6685e502-776e-4f88-b659-97fdff9973de', sdkVersion: '3',
                   targetPlatforms: ['basalt', 'chalk', 'diorite', 'emery', 'flint', 'gabbro'],
                   capabilities: ['configurable', 'microphone']}
        }, null, 2));
      }
      return 'cat: ' + escapeHtml(args || '') + ': No such file or directory';
    },
    ls: function() { return '<span class="t-d">src/</span>  <span class="t-d">resources/</span>  <span class="t-d">store/</span>  <span class="t-d">test/</span>  LICENSE  README.md  package.json  wscript'; },
    theme: function(args) {
      if (themes.indexOf(args) < 0) return 'theme: ' + theme + '  (auto|light|dark)';
      theme = args;
      applyTheme(theme);
      store('ostrakon-theme', theme === 'auto' ? null : theme);
      return 'theme set to ' + theme;
    },
    whoami: function() { return 'memo'; },
    date: function() { return new Date().toString(); },
    echo: function(args) { return escapeHtml(args); },
    uname: function() { return 'PebbleOS 4 armv7m (pretend)'; },
    pwd: function() { return '/home/memo'; },
    clear: function() { out.innerHTML = ''; return null; },
    exit: function() { closeShell(); return null; },
    vim: function() { return 'you are now trapped. just kidding: type <span class="t-p">exit</span>'; },
    emacs: function() { return 'nice operating system, but it does not fit on a watch.'; },
    sudo: function(args) {
      if (/sandwich/.test(args)) return 'okay.  🥪';
      return 'memo is not in the sudoers file. This incident will be scratched onto a shard.';
    },
    rm: function() { return 'rm: refusing to break the pottery further'; },
    ostracize: function(args) {
      return args ? escapeHtml(args) + ' has been voted out of Athens for ten years.' : 'usage: ostracize &lt;name&gt;';
    }
  };

  function run(line) {
    var trimmed = line.trim();
    print('<span class="t-p">memo@pebble:~$</span> ' + escapeHtml(line));
    if (!trimmed) return;
    var space = trimmed.indexOf(' ');
    var cmd = (space < 0 ? trimmed : trimmed.slice(0, space)).toLowerCase();
    var args = space < 0 ? '' : trimmed.slice(space + 1).trim();
    var fn = COMMANDS[cmd];
    var result = fn ? fn(args) : '<span class="t-e">' + escapeHtml(cmd) + ': command not found</span>. try <span class="t-p">help</span>';
    if (result !== null) print(result);
  }

  function openShell() {
    if (!shell.hidden) return;
    lastFocus = document.activeElement;
    shell.hidden = false;
    if (!out.innerHTML) {
      print('<span class="t-o">ostrakon-sh</span> 1.0 · type <span class="t-p">help</span> to get started\n');
    }
    input.focus();
  }
  function closeShell() {
    shell.hidden = true;
    if (lastFocus && lastFocus.focus) lastFocus.focus();
  }

  $('#term-open').addEventListener('click', openShell);
  $('#shell-close').addEventListener('click', closeShell);
  shell.addEventListener('click', function(e) { if (e.target === shell) closeShell(); });
  $('#shell-form').addEventListener('submit', function(e) {
    e.preventDefault();
    var line = input.value;
    if (line.trim()) history.push(line);
    histPos = history.length;
    input.value = '';
    run(line);
  });
  input.addEventListener('keydown', function(e) {
    if (e.key === 'ArrowUp' && histPos > 0) {
      input.value = history[--histPos];
      e.preventDefault();
    } else if (e.key === 'ArrowDown') {
      histPos = Math.min(history.length, histPos + 1);
      input.value = history[histPos] || '';
      e.preventDefault();
    } else if (e.key === 'Tab') {
      e.preventDefault();
      var matches = Object.keys(COMMANDS).filter(function(c) { return c.indexOf(input.value) === 0; });
      if (matches.length === 1) input.value = matches[0] + ' ';
      else if (matches.length > 1) print(matches.join('  '));
    }
  });

  /* =========================================================
   * Keyboard: ` opens the shell, arrows drive the watch,
   * and the Konami code breaks some pottery
   * ========================================================= */

  var KONAMI = ['ArrowUp', 'ArrowUp', 'ArrowDown', 'ArrowDown', 'ArrowLeft', 'ArrowRight',
                'ArrowLeft', 'ArrowRight', 'b', 'a'];
  var konamiPos = 0;

  document.addEventListener('keydown', function(e) {
    var typing = /^(INPUT|TEXTAREA|SELECT)$/.test(e.target.tagName) || e.target.isContentEditable;
    if (e.key === 'Escape' && !shell.hidden) return closeShell();
    if (typing || e.metaKey || e.ctrlKey || e.altKey) return;

    var key = e.key.length === 1 ? e.key.toLowerCase() : e.key;
    konamiPos = key === KONAMI[konamiPos] ? konamiPos + 1 : (key === KONAMI[0] ? 1 : 0);
    if (konamiPos === KONAMI.length) {
      konamiPos = 0;
      shatter();
      return;
    }

    if (e.key === '`' || e.key === '~') {
      e.preventDefault();
      openShell();
    } else if (e.key === 'ArrowUp' || e.key === 'ArrowDown') {
      var rect = $('#hero-watch').getBoundingClientRect();
      if (rect.bottom > 0 && rect.top < window.innerHeight) {
        e.preventDefault();
        autoplay(false);
        showHero(heroIndex + (e.key === 'ArrowUp' ? -1 : 1));
      }
    }
  });

  // A rain of little shards
  function shatter() {
    var canvas = $('#confetti');
    var ctx = canvas.getContext('2d');
    var dpr = window.devicePixelRatio || 1;
    canvas.width = innerWidth * dpr;
    canvas.height = innerHeight * dpr;
    ctx.scale(dpr, dpr);
    var colors = ['#ff5500', '#ff8a4d', '#111111', '#9a9a9a'];
    var bits = [];
    for (var i = 0; i < 140; i++) {
      var pts = [];
      var n = 4 + Math.floor(Math.random() * 3);
      var size = 5 + Math.random() * 12;
      for (var k = 0; k < n; k++) {
        var a = k / n * Math.PI * 2 + Math.random() * 0.6;
        pts.push([Math.cos(a) * size * (0.6 + Math.random() * 0.4), Math.sin(a) * size * (0.6 + Math.random() * 0.4)]);
      }
      bits.push({
        x: innerWidth / 2 + (Math.random() - 0.5) * 120, y: innerHeight / 3,
        vx: (Math.random() - 0.5) * 14, vy: -Math.random() * 14 - 4,
        r: Math.random() * 6, vr: (Math.random() - 0.5) * 0.3,
        pts: pts, c: colors[i % colors.length]
      });
    }
    var start = performance.now();
    (function frame(now) {
      ctx.clearRect(0, 0, innerWidth, innerHeight);
      bits.forEach(function(b) {
        b.vy += 0.45; b.x += b.vx; b.y += b.vy; b.r += b.vr; b.vx *= 0.99;
        ctx.save();
        ctx.translate(b.x, b.y);
        ctx.rotate(b.r);
        ctx.beginPath();
        b.pts.forEach(function(p, j) { j ? ctx.lineTo(p[0], p[1]) : ctx.moveTo(p[0], p[1]); });
        ctx.closePath();
        ctx.fillStyle = b.c;
        ctx.fill();
        ctx.restore();
      });
      if (now - start < 4000) requestAnimationFrame(frame);
      else ctx.clearRect(0, 0, innerWidth, innerHeight);
    })(start);
    if (!shell.hidden) print('<span class="t-o">+30 lives. the pottery did not survive.</span>');
  }
})();
