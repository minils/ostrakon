#pragma once
#include <pebble.h>

/* ---------- Buffer sizes (must match the limits in index.js) ---------- */
#define COMM_ID_SIZE 48
#define COMM_TITLE_SIZE 64
#define COMM_SUBTITLE_SIZE 48
#define COMM_TEXT_SIZE 1536
#define COMM_MESSAGE_SIZE 1024

/* ---------- Actions (must match index.js) ---------- */
typedef enum {
  CommActionCreate = 1,
  CommActionListRecent = 2,
  CommActionListTags = 3,
  CommActionListByTag = 4,
  CommActionGet = 5,
  CommActionArchive = 6,
  CommActionDelete = 7,
  CommActionComment = 8,
  CommActionListTasks = 9,
  CommActionToggleTask = 10,
  CommActionAppend = 11,
  CommActionSearch = 12,
  CommActionListPinned = 13,
  CommActionTogglePin = 14,
  CommActionListComments = 15,
} CommAction;

/* ---------- Flags of memo list items (must match index.js) ---------- */
#define MEMO_FLAG_HAS_COMMENTS 2
// the text of a memo continues on the next page (only for CommActionGet)
#define MEMO_FLAG_MORE_TEXT 4

// Called for every response message of the current request. iter is NULL
// if the phone could not be reached or did not answer in time.
typedef void (*CommHandler)(DictionaryIterator *iter, void *context);

void comm_init(void);

// Sends a request, replacing any pending one. Returns false if the message
// could not be sent; the handler is not called in that case.
bool comm_request(CommAction action, const char *target, const char *text,
                  CommHandler handler, void *context);

// Same for a list request, asking for the page starting at offset
bool comm_request_page(CommAction action, const char *target, const char *text,
                       int32_t offset, CommHandler handler, void *context);

// Stops delivering responses of the pending request
void comm_finish(void);

// True while a request with this handler context waits for responses,
// i.e. it was not finished or replaced by another request
bool comm_is_pending(void *context);

bool comm_response_failed(DictionaryIterator *iter);
const char *comm_response_error(DictionaryIterator *iter);
int32_t comm_get_int(DictionaryIterator *iter, uint32_t key, int32_t fallback);
bool comm_copy_string(DictionaryIterator *iter, uint32_t key, char *buffer,
                      size_t size);
