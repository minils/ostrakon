// Runs inside the settings page (Clay copies this function into it, so it
// must not use anything from outside). Wires up the "Test connection"
// button and the rows of own keywords. The server may not allow requests
// from this page (CORS); the watch also reports the connection status after
// saving.
module.exports = function() {
  var clayConfig = this;
  // the page strings of the current language, set in index.js
  var t = (clayConfig.meta.userData || {}).strings || {};

  // Own keywords: a row of word and markdown each, added with "+" and
  // removed with "-". Clay has no such list, so the rows are built here
  // and kept as JSON in the hidden input of the "keywords" item.
  function buildKeywordRows() {
    var item = clayConfig.getItemById('keywords');
    var root = item.$element[0];
    var input = root.querySelector('.input');
    input.style.display = 'none';
    var list = document.createElement('div');
    root.insertBefore(list, root.querySelector('.description'));

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
      [[t.keywordWord, keyword.word], [t.keywordMarkdown, keyword.markdown]]
          .forEach(function(field) {
            var input = document.createElement('input');
            input.type = 'text';
            input.placeholder = field[0] || '';
            input.value = field[1] || '';
            input.style.flex = '1';
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

    var add = button('+', t.keywordAdd || '+');
    add.style.marginTop = '0.7rem';
    add.addEventListener('click', function() {
      addRow({}).querySelector('input').focus();
    });
    list.appendChild(add);

    var keywords = [];
    try {
      keywords = JSON.parse(item.get() || '[]');
    } catch (e) {
    }
    // one empty row shows what to fill in
    if (!keywords.length) keywords = [{}];
    keywords.forEach(addRow);
  }

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    buildKeywordRows();

    var result = clayConfig.getItemById('testResult');

    function show(text, color) {
      result.set('<span style="color:' + color + '">' + text + '</span>');
    }

    clayConfig.getItemById('testConnection').on('click', function() {
      var url =
          String(clayConfig.getItemByMessageKey('SETTING_URL').get() || '')
              .trim()
              .replace(/\/+$/, '')
              .replace(/\/api\/v1\/memos$/, '');
      var token =
          String(clayConfig.getItemByMessageKey('SETTING_TOKEN').get() || '')
              .trim();
      if (!url || !token) {
        show(t.testMissing, '#ff8a80');
        return;
      }

      show(t.testRunning, '#cccccc');
      var xhr = new XMLHttpRequest();
      xhr.open('GET', url + '/api/v1/memos?pageSize=1', true);
      xhr.setRequestHeader('Authorization', 'Bearer ' + token);
      xhr.timeout = 10000;
      xhr.onload = function() {
        if (xhr.status >= 200 && xhr.status < 300) {
          show(t.testOk, '#69f0ae');
        } else if (xhr.status === 401 || xhr.status === 403) {
          show(t.testRejected.replace('%s', xhr.status), '#ff8a80');
        } else if (xhr.status === 404) {
          show(t.testNotFound, '#ff8a80');
        } else {
          show(t.testServerError.replace('%s', xhr.status), '#ff8a80');
        }
      };
      xhr.onerror = xhr.ontimeout = function() {
        show(t.testUnavailable, '#cccccc');
      };
      xhr.send();
    });
  });
};
