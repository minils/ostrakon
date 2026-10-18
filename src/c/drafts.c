#include "drafts.h"

#include "comm.h"
#include "ui.h"

/* ---------- Persistent storage keys ---------- */
// Each draft is split across several keys because a single persisted value
// is limited to PERSIST_DATA_MAX_LENGTH (256) bytes. Draft 0 uses the keys
// of the single draft stored by earlier versions.
#define PERSIST_KEY_DRAFT_BASE 100
#define DRAFT_CHUNK_SIZE PERSIST_DATA_MAX_LENGTH
#define DRAFT_NUM_CHUNKS (COMM_MESSAGE_SIZE / DRAFT_CHUNK_SIZE)
#define DRAFT_KEY(draft, chunk) \
  (PERSIST_KEY_DRAFT_BASE + (draft) * DRAFT_NUM_CHUNKS + (chunk))

// the drafts are read from storage when needed, not kept in memory
static int s_count;

// Storage

static bool write_draft(int draft, const char *text) {
  size_t len = strlen(text) + 1;  // include the null terminator
  for (int i = 0; i < (int)DRAFT_NUM_CHUNKS; i++) {
    size_t offset = i * DRAFT_CHUNK_SIZE;
    if (offset < len) {
      size_t chunk_len = len - offset;
      if (chunk_len > DRAFT_CHUNK_SIZE) chunk_len = DRAFT_CHUNK_SIZE;
      if (persist_write_data(DRAFT_KEY(draft, i), text + offset, chunk_len) <
          0) {
        return false;
      }
    } else {
      persist_delete(DRAFT_KEY(draft, i));
    }
  }
  return true;
}

static void delete_stored_draft(int draft) {
  for (int i = 0; i < (int)DRAFT_NUM_CHUNKS; i++) {
    persist_delete(DRAFT_KEY(draft, i));
  }
}

// Moves the stored draft from to the keys of draft to, a chunk at a time
static void move_stored_draft(int from, int to) {
  char chunk[DRAFT_CHUNK_SIZE];
  for (int i = 0; i < (int)DRAFT_NUM_CHUNKS; i++) {
    int size = persist_get_size(DRAFT_KEY(from, i));
    if (size > 0) {
      persist_read_data(DRAFT_KEY(from, i), chunk, sizeof(chunk));
      persist_write_data(DRAFT_KEY(to, i), chunk, size);
    } else {
      persist_delete(DRAFT_KEY(to, i));
    }
  }
  delete_stored_draft(from);
}

// Public api

void drafts_init(void) {
  s_count = 0;
  while (s_count < DRAFTS_MAX && persist_exists(DRAFT_KEY(s_count, 0))) {
    s_count++;
  }
}

int drafts_count(void) { return s_count; }

void drafts_read(int index, char *buffer, size_t size) {
  buffer[0] = '\0';
  if (index < 0 || index >= s_count || size == 0) {
    return;
  }
  size_t limit = size - 1;
  if (limit > COMM_MESSAGE_SIZE - 1) {
    limit = COMM_MESSAGE_SIZE - 1;
  }
  for (int i = 0; i < (int)DRAFT_NUM_CHUNKS; i++) {
    size_t offset = i * DRAFT_CHUNK_SIZE;
    if (offset >= limit || !persist_exists(DRAFT_KEY(index, i))) break;
    size_t chunk_len = limit - offset;
    if (chunk_len > DRAFT_CHUNK_SIZE) chunk_len = DRAFT_CHUNK_SIZE;
    persist_read_data(DRAFT_KEY(index, i), buffer + offset, chunk_len);
  }
  buffer[limit] = '\0';
  // a cut text must not end in the middle of a UTF-8 character
  size_t len = strlen(buffer);
  if (len == limit) {
    while (len > 0 && ((uint8_t)buffer[len] & 0xC0) == 0x80) {
      len--;
    }
    buffer[len] = '\0';
  }
}

int drafts_save(int index, const char *text) {
  if (index < 0 || index >= s_count) {
    if (s_count >= DRAFTS_MAX) {
      return -1;
    }
    index = s_count++;
  }
  if (!write_draft(index, text)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Could not persist draft %d", index);
  }
  return index;
}

void drafts_delete(int index) {
  if (index < 0 || index >= s_count) {
    return;
  }
  // keep the stored drafts contiguous
  delete_stored_draft(index);
  for (int i = index; i < s_count - 1; i++) {
    move_stored_draft(i + 1, i);
  }
  s_count--;
}
