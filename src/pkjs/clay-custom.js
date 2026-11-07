// Runs inside the settings page (Clay copies this function into it, so it
// must not use anything from outside). Wires up the "Test connection"
// button and the rows of own keywords. The server may not allow requests
// from this page (CORS); the watch also reports the connection status after
// saving.
module.exports = function() {
  var clayConfig = this;
  // the page strings of the current language, set in index.js
  var t = (clayConfig.meta.userData || {}).strings || {};

  // Keywords: a row of word and markdown each, added with "+", removed
  // with "-", and reset to the defaults of the language. Clay has no such
  // list, so the rows are built here and kept as JSON in the hidden input
  // of the "keywords" item.
  function buildKeywordRows() {
    var item = clayConfig.getItemById('keywords');
    var root = item.$element[0];
    var input = root.querySelector('.input');
    input.style.display = 'none';
    var list = document.createElement('div');
    // indented like the label and the inputs of the other items
    list.style.padding = '0 0.75rem 0.7rem';
    root.insertBefore(list, root.querySelector('.description'));

    // accents in Clay's orange: the markdown and the field being edited
    var style = document.createElement('style');
    style.textContent = '.keyword-row .keyword-markdown { color: #ff4700; ' +
        'font-family: monospace; }' +
        '.keyword-row input:focus { box-shadow: inset 0 -2px #ff4700; }';
    root.appendChild(style);

    function button(text, title) {
      var b = document.createElement('button');
      b.type = 'button';
      b.textContent = text;
      b.title = title;
      b.setAttribute('aria-label', title);
      b.style.minWidth = '2.1rem';
      b.style.margin = '0';
      b.style.padding = '0.35rem 0.6rem';
      return b;
    }

    function save() {
      var rows = [];
      var fields = list.querySelectorAll('.keyword-row');
      for (var i = 0; i < fields.length; i++) {
        var inputs = fields[i].querySelectorAll('input');
        rows.push({word: inputs[0].value, markdown: inputs[1].value});
      }
      item.set(JSON.stringify(rows));
    }

    function addRow(keyword) {
      var row = document.createElement('div');
      row.className = 'keyword-row';
      row.style.display = 'flex';
      row.style.marginTop = '0.7rem';
      // the markdown is mostly short, so the word gets more room
      [[t.keywordWord, keyword.word, '3', 'keyword-word'],
       [t.keywordMarkdown, keyword.markdown, '2', 'keyword-markdown']]
          .forEach(function(field) {
            var input = document.createElement('input');
            input.type = 'text';
            input.className = field[3];
            input.placeholder = field[0] || '';
            input.value = field[1] || '';
            input.style.flex = field[2];
            input.style.minWidth = '0';
            input.style.marginRight = '0.5rem';
            input.addEventListener('input', save);
            row.appendChild(input);
          });
      var remove = button('\u2212', t.keywordRemove || '-');
      remove.addEventListener('click', function() {
        list.removeChild(row);
        if (!list.querySelector('.keyword-row')) addRow({});
        save();
      });
      row.appendChild(remove);
      list.insertBefore(row, add);
      return row;
    }

    function showRows(keywords) {
      var rows = list.querySelectorAll('.keyword-row');
      for (var i = 0; i < rows.length; i++) list.removeChild(rows[i]);
      // one empty row shows what to fill in
      (keywords.length ? keywords : [{}]).forEach(addRow);
    }

    var buttons = document.createElement('div');
    buttons.style.display = 'flex';
    buttons.style.justifyContent = 'space-between';
    buttons.style.marginTop = '0.7rem';
    var add = button('+', t.keywordAdd || '+');
    // orange like Clay's save button
    add.className = 'primary';
    add.addEventListener('click', function() {
      addRow({}).querySelector('input').focus();
    });
    var reset = button(t.keywordsReset || 'Reset', t.keywordsReset || 'Reset');
    reset.addEventListener('click', function() {
      showRows((clayConfig.meta.userData || {}).defaultKeywords || []);
      save();
    });
    buttons.appendChild(add);
    buttons.appendChild(reset);
    list.appendChild(buttons);
    // rows go above the buttons
    add = buttons;

    var keywords = [];
    try {
      keywords = JSON.parse(item.get() || '[]');
    } catch (e) {
    }
    showRows(keywords);
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    buildKeywordRows();

    var result = clayConfig.getItemById('testResult');

    function show(text, color) {
      result.set('<span style="color:' + color + '">' + text + '</span>');
    }

    // The page is a data: URL, so its requests have the origin "null",
    // which Memos does not allow for CORS: the page cannot read any answer
    // and so cannot check the token. A no-cors request still tells whether
    // the server answers at all; the watch checks the token after saving.
    // Browsers may block requests from such a page to the local network, so
    // a server at home can fail here and still work from the watch.
    clayConfig.getItemById('testConnection').on('click', function() {
      var url =
          String(clayConfig.getItemByMessageKey('SETTING_URL').get() || '')
              .trim()
              .replace(/\/+$/, '')
              .replace(/\/api(\/v1)?\/memos$/, '');
      if (!url) {
        show(t.testMissing, '#ff8a80');
        return;
      }

      show(t.testRunning, '#cccccc');
      var done = false;
      function finish(reachable) {
        if (done) return;
        done = true;
        if (reachable)
          show(t.testOk, '#69f0ae');
        else
          show(t.testUnreachable, '#ff8a80');
      }
      setTimeout(function() {
        finish(false);
      }, 10000);
      try {
        fetch(url + '/', {mode: 'no-cors', cache: 'no-store'})
            .then(
                function() {
                  finish(true);
                },
                function() {
                  finish(false);
                });
      } catch (err) {
        // an invalid URL
        finish(false);
      }
    });
  });
};
