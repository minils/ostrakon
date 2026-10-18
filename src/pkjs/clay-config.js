// Layout of the settings page, built by Clay (see vendor/clay.js), with
// the texts of the given page strings (see i18n.js). The message keys name
// the settings; they are read in index.js and are not sent to the watch as
// they are.
module.exports = function(t) {
  return [
    {'type': 'heading', 'defaultValue': 'Ostrakon'},
    {'type': 'text', 'defaultValue': t.tagline}, {
      'type': 'section',
      'items': [
        {'type': 'heading', 'defaultValue': t.connection}, {
          'type': 'input',
          'messageKey': 'SETTING_URL',
          'label': t.url,
          'description': t.urlHint,
          'attributes':
              {'placeholder': 'https://memos.example.com', 'type': 'url'}
        },
        {
          'type': 'input',
          'messageKey': 'SETTING_TOKEN',
          'label': t.token,
          'description': t.tokenHint,
          'attributes': {'placeholder': t.tokenPlaceholder, 'type': 'password'}
        },
        {'type': 'button', 'id': 'testConnection', 'defaultValue': t.test},
        {'type': 'text', 'id': 'testResult', 'defaultValue': ''}
      ]
    },
    {
      'type': 'section',
      'items': [
        {'type': 'heading', 'defaultValue': t.memos}, {
          'type': 'input',
          'messageKey': 'SETTING_TAGS',
          'label': t.tags,
          'description': t.tagsHint,
          'defaultValue': '#pebble',
          'attributes': {'placeholder': '#pebble #notes'}
        },
        {
          'type': 'input',
          'messageKey': 'SETTING_TODO_TAG',
          'label': t.todoTag,
          'description': t.todoTagHint,
          'defaultValue': '#todo',
          'attributes': {'placeholder': '#todo'}
        }
      ]
    },
    {
      'type': 'section',
      'items': [
        {'type': 'heading', 'defaultValue': t.watch}, {
          'type': 'toggle',
          'messageKey': 'VIBRATE',
          'label': t.vibrate,
          'defaultValue': true
        },
        {
          'type': 'select',
          'messageKey': 'SETTING_LANGUAGE',
          'label': t.language,
          'defaultValue': 'auto',
          'options': [
            {'label': t.languageAuto, 'value': 'auto'},
            {'label': 'English', 'value': 'en'},
            {'label': 'Deutsch', 'value': 'de'}
          ]
        }
      ]
    },
    {'type': 'submit', 'defaultValue': t.save}
  ];
};
