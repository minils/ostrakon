// Runs inside the settings page (Clay copies this function into it, so it
// must not use anything from outside). Wires up the "Test connection"
// button. The server may not allow requests from this page (CORS); the
// watch also reports the connection status after saving.
module.exports = function() {
  var clayConfig = this;
  // the page strings of the current language, set in index.js
  var t = (clayConfig.meta.userData || {}).strings || {};

  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
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
