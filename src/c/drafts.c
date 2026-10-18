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

static char s_drafts[DRAFTS_MAX][COMM_MESSAGE_SIZE];
static int s_count;

// Storage

static bool write_draft(int draft) {
  const char *text = s_drafts[draft];
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

static bool read_draft(int draft) {
  if (!persist_exists(DRAFT_KEY(draft, 0))) {
    return false;
  }
  char *text = s_drafts[draft];
  for (int i = 0; i < (int)DRAFT_NUM_CHUNKS; i++) {
    if (!persist_exists(DRAFT_KEY(draft, i))) break;
    persist_read_data(DRAFT_KEY(draft, i), text + i * DRAFT_CHUNK_SIZE,
                      DRAFT_CHUNK_SIZE);
  }
  text[COMM_MESSAGE_SIZE - 1] = '\0';
  return true;
}

// Public api

void drafts_init(void) {
  s_count = 0;
  while (s_count < DRAFTS_MAX && read_draft(s_count)) {
    s_count++;
  }
}

int drafts_count(void) { return s_count; }

const char *drafts_get(int index) {
  return (index >= 0 && index < s_count) ? s_drafts[index] : "";
}

int drafts_save(int index, const char *text) {
  if (index < 0 || index >= s_count) {
    if (s_count >= DRAFTS_MAX) {
      return -1;
    }
    index = s_count++;
  }
  ui_copy_truncated(s_drafts[index], text, COMM_MESSAGE_SIZE);
  if (!write_draft(index)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Could not persist draft %d", index);
  }
  return index;
}

void drafts_delete(int index) {
  if (index < 0 || index >= s_count) {
    return;
  }
  // keep the drafts contiguous so the stored layout matches s_drafts
  for (int i = index; i < s_count - 1; i++) {
    strcpy(s_drafts[i], s_drafts[i + 1]);
    write_draft(i);
  }
  s_count--;
  s_drafts[s_count][0] = '\0';
  delete_stored_draft(s_count);
}
