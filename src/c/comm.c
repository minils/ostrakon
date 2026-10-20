#include "comm.h"

#include "i18n.h"
#include "settings.h"
#include "toast.h"

#define COMM_TIMEOUT_MS 20000

static uint32_t s_request_id;
static CommHandler s_handler;
static void *s_context;
static AppTimer *s_timeout_timer;

// Pending request

void comm_finish(void) {
  s_handler = NULL;
  s_context = NULL;
  if (s_timeout_timer) {
    app_timer_cancel(s_timeout_timer);
    s_timeout_timer = NULL;
  }
}

bool comm_is_pending(void *context) {
  return s_handler && s_context == context;
}

static void fail_pending(void) {
  CommHandler handler = s_handler;
  void *context = s_context;
  comm_finish();
  if (handler) {
    handler(NULL, context);
  }
}

static void timeout_callback(void *data) {
  s_timeout_timer = NULL;
  fail_pending();
}

static void restart_timeout(void) {
  if (s_timeout_timer) {
    app_timer_reschedule(s_timeout_timer, COMM_TIMEOUT_MS);
  } else {
    s_timeout_timer =
        app_timer_register(COMM_TIMEOUT_MS, timeout_callback, NULL);
  }
}

// Sending

bool comm_request(CommAction action, const char *target, const char *text,
                  CommHandler handler, void *context) {
  return comm_request_page(action, target, text, -1, 0, handler, context);
}

bool comm_request_page(CommAction action, const char *target, const char *text,
                       int32_t offset, int32_t version, CommHandler handler,
                       void *context) {
  comm_finish();

  // fail right away instead of waiting for the timeout
  if (!connection_service_peek_pebble_app_connection()) {
    return false;
  }

  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return false;
  }

  s_request_id++;
  dict_write_int32(iter, MESSAGE_KEY_REQUEST_ID, (int32_t)s_request_id);
  dict_write_int32(iter, MESSAGE_KEY_ACTION, action);
  if (target) {
    dict_write_cstring(iter, MESSAGE_KEY_ITEM_ID, target);
  }
  if (text) {
    dict_write_cstring(iter, MESSAGE_KEY_TEXT, text);
  }
  if (offset >= 0) {
    dict_write_int32(iter, MESSAGE_KEY_ITEM_INDEX, offset);
  }
  if (version != 0) {
    dict_write_int32(iter, MESSAGE_KEY_CACHE_VERSION, version);
  }

  if (app_message_outbox_send() != APP_MSG_OK) {
    return false;
  }

  s_handler = handler;
  s_context = context;
  restart_timeout();
  return true;
}

// Receiving

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  // settings and notices are pushed by the phone outside of any request
  int32_t vibrate = comm_get_int(iter, MESSAGE_KEY_VIBRATE, -1);
  if (vibrate >= 0) {
    settings_set_vibrate(vibrate != 0);
  }
  int32_t language = comm_get_int(iter, MESSAGE_KEY_LANGUAGE, -1);
  if (language >= 0) {
    i18n_set_language(language);
  }
  int32_t glance_count = comm_get_int(iter, MESSAGE_KEY_GLANCE_COUNT, -1);
  if (glance_count >= 0) {
    settings_set_glance_count(glance_count);
  }
  Tuple *notice = dict_find(iter, MESSAGE_KEY_NOTICE);
  if (notice && notice->type == TUPLE_CSTRING) {
    toast_show(notice->value->cstring, !comm_response_failed(iter));
    return;
  }

  // responses to replaced requests are ignored
  if (!s_handler ||
      comm_get_int(iter, MESSAGE_KEY_REQUEST_ID, -1) != (int32_t)s_request_id) {
    return;
  }
  restart_timeout();
  s_handler(iter, s_context);
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Inbox dropped: %d", (int)reason);
  fail_pending();
}

static void outbox_failed_handler(DictionaryIterator *iter,
                                  AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "Outbox failed: %d", (int)reason);
  fail_pending();
}

// Helpers

int32_t comm_get_int(DictionaryIterator *iter, uint32_t key, int32_t fallback) {
  Tuple *tuple = dict_find(iter, key);
  if (!tuple) {
    return fallback;
  }
  switch (tuple->length) {
    case 1:
      return tuple->type == TUPLE_INT ? tuple->value->int8
                                      : tuple->value->uint8;
    case 2:
      return tuple->type == TUPLE_INT ? tuple->value->int16
                                      : tuple->value->uint16;
    case 4:
      return tuple->value->int32;
    default:
      return fallback;
  }
}

bool comm_copy_string(DictionaryIterator *iter, uint32_t key, char *buffer,
                      size_t size) {
  Tuple *tuple = dict_find(iter, key);
  if (!tuple || tuple->type != TUPLE_CSTRING) {
    buffer[0] = '\0';
    return false;
  }
  strncpy(buffer, tuple->value->cstring, size - 1);
  buffer[size - 1] = '\0';
  return true;
}

bool comm_response_failed(DictionaryIterator *iter) {
  return comm_get_int(iter, MESSAGE_KEY_STATUS, 0) != 0;
}

bool comm_response_is_warning(DictionaryIterator *iter) {
  return comm_get_int(iter, MESSAGE_KEY_STATUS, 0) == 2;
}

bool comm_response_unchanged(DictionaryIterator *iter, int32_t version) {
  return version != 0 &&
         comm_get_int(iter, MESSAGE_KEY_CACHE_VERSION, 0) == version;
}

const char *comm_response_error(DictionaryIterator *iter) {
  Tuple *tuple = dict_find(iter, MESSAGE_KEY_TEXT);
  if (tuple && tuple->type == TUPLE_CSTRING) {
    return tuple->value->cstring;
  }
  return tr("Request failed");
}

void comm_init(void) {
  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);

  // inbox must fit a full memo text, outbox a full dictated message
  uint32_t inbox_size = dict_calc_buffer_size(
      4, sizeof(int32_t), sizeof(int32_t), sizeof(int32_t), COMM_TEXT_SIZE);
  uint32_t outbox_size =
      dict_calc_buffer_size(5, sizeof(int32_t), sizeof(int32_t), COMM_ID_SIZE,
                            COMM_MESSAGE_SIZE, sizeof(int32_t));
  app_message_open(inbox_size, outbox_size);
}
