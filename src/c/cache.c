#include "cache.h"

#include "drafts.h"
#include "i18n.h"

/* ---------- Persistent storage keys ---------- */
// An index of the entries, most recently stored first, and the chunks
// holding their data, each up to PERSIST_DATA_MAX_LENGTH bytes
#define PERSIST_KEY_CACHE_INDEX 300
#define PERSIST_KEY_CACHE_CHUNK 301
#define CHUNK_SIZE PERSIST_DATA_MAX_LENGTH
#define ENTRY_CHUNKS (CACHE_ENTRY_MAX / CHUNK_SIZE)
#define MAX_CHUNKS 64
#define MAX_ENTRIES 12

// apps built with older SDKs get this much
#define DEFAULT_STORAGE_SIZE 4096
// room for the settings and the storage's own bookkeeping
#define STORAGE_MARGIN 512
#define KEY_OVERHEAD 8

typedef struct {
  uint32_t key_hash;
  uint32_t data_hash;  // to skip writing data that did not change
  uint16_t size;
  int16_t flags;
  uint8_t chunks[ENTRY_CHUNKS];
} CacheEntry;

typedef struct {
  uint8_t count;
  CacheEntry entries[MAX_ENTRIES];
} CacheIndex;

_Static_assert(sizeof(CacheIndex) <= PERSIST_DATA_MAX_LENGTH,
               "the cache index must fit one persisted value");

// FNV-1a
static uint32_t hash(const char *data, size_t size) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < size; i++) {
    h = (h ^ (uint8_t)data[i]) * 16777619u;
  }
  return h;
}

// The phone formats the cached texts, e.g. dates, in the current language,
// so an entry belongs to the language it was stored in; after a language
// change the old entries are not found and make room for new ones
static uint32_t key_hash(const char *key, Language language) {
  return (hash(key, strlen(key)) ^ language) * 16777619u;
}

static void read_index(CacheIndex *index) {
  if (persist_read_data(PERSIST_KEY_CACHE_INDEX, index, sizeof(*index)) !=
          (int)sizeof(*index) ||
      index->count > MAX_ENTRIES) {
    index->count = 0;
  }
}

static void write_index(const CacheIndex *index) {
  persist_write_data(PERSIST_KEY_CACHE_INDEX, index, sizeof(*index));
}

static int num_chunks(const CacheEntry *entry) {
  return (entry->size + CHUNK_SIZE - 1) / CHUNK_SIZE;
}

static size_t entry_bytes(const CacheEntry *entry) {
  return entry->size + num_chunks(entry) * KEY_OVERHEAD;
}

static size_t index_bytes(const CacheIndex *index) {
  size_t bytes = sizeof(*index) + KEY_OVERHEAD;
  for (int i = 0; i < index->count; i++) {
    bytes += entry_bytes(&index->entries[i]);
  }
  return bytes;
}

static size_t storage_size(void) {
#if PBL_API_EXISTS(persist_get_max_size)
  return persist_get_max_size();
#else
  return DEFAULT_STORAGE_SIZE;
#endif
}

// what the cache may take next to others bytes of other data
static size_t budget(size_t others) {
  size_t reserved = others + STORAGE_MARGIN;
  size_t total = storage_size();
  return total > reserved ? total - reserved : 0;
}

static void remove_entry(CacheIndex *index, int i) {
  CacheEntry *entry = &index->entries[i];
  for (int c = 0; c < num_chunks(entry); c++) {
    persist_delete(PERSIST_KEY_CACHE_CHUNK + entry->chunks[c]);
  }
  memmove(entry, entry + 1, (index->count - i - 1) * sizeof(CacheEntry));
  index->count--;
}

static int find(const CacheIndex *index, uint32_t key_hash) {
  for (int i = 0; i < index->count; i++) {
    if (index->entries[i].key_hash == key_hash) {
      return i;
    }
  }
  return -1;
}

// drops the oldest entries until bytes more fit
static void shrink(CacheIndex *index, size_t bytes, size_t limit) {
  while (index->count > 0 && index_bytes(index) + bytes > limit) {
    remove_entry(index, index->count - 1);
  }
}

// a chunk neither the entries nor the first taken chunks of a new one use
static int free_chunk(const CacheIndex *index, const uint8_t *taken,
                      int num_taken) {
  for (int chunk = 0; chunk < MAX_CHUNKS; chunk++) {
    bool used = false;
    for (int i = 0; i < index->count && !used; i++) {
      const CacheEntry *entry = &index->entries[i];
      for (int c = 0; c < num_chunks(entry); c++) {
        used = used || entry->chunks[c] == chunk;
      }
    }
    for (int c = 0; c < num_taken; c++) {
      used = used || taken[c] == chunk;
    }
    if (!used) {
      return chunk;
    }
  }
  return -1;
}

// Public api

void cache_store(const char *key, int32_t flags, const char *data,
                 size_t size) {
  if (size > CACHE_ENTRY_MAX) {
    size = CACHE_ENTRY_MAX;
  }
  CacheIndex index;
  read_index(&index);
  uint32_t entry_key = key_hash(key, i18n_language());
  uint32_t data_hash = hash(data, size);

  int existing = find(&index, entry_key);
  if (existing >= 0) {
    CacheEntry entry = index.entries[existing];
    if (entry.data_hash == data_hash && entry.size == size &&
        entry.flags == flags) {
      // unchanged, only move it to the front
      memmove(&index.entries[1], &index.entries[0],
              existing * sizeof(CacheEntry));
      index.entries[0] = entry;
      write_index(&index);
      return;
    }
    remove_entry(&index, existing);
  }

  CacheEntry entry = {
      .key_hash = entry_key,
      .data_hash = data_hash,
      .size = size,
      .flags = flags,
  };
  if (index.count == MAX_ENTRIES) {
    remove_entry(&index, index.count - 1);
  }
  shrink(&index, entry_bytes(&entry), budget(drafts_stored_size()));
  if (index_bytes(&index) + entry_bytes(&entry) >
      budget(drafts_stored_size())) {
    write_index(&index);
    return;  // no room even when empty
  }

  for (int c = 0; c < num_chunks(&entry); c++) {
    int chunk = free_chunk(&index, entry.chunks, c);
    size_t offset = c * CHUNK_SIZE;
    size_t len = size - offset < CHUNK_SIZE ? size - offset : CHUNK_SIZE;
    if (chunk < 0 || persist_write_data(PERSIST_KEY_CACHE_CHUNK + chunk,
                                        data + offset, len) < (int)len) {
      // drop what was written of this entry
      for (int w = 0; w < c; w++) {
        persist_delete(PERSIST_KEY_CACHE_CHUNK + entry.chunks[w]);
      }
      write_index(&index);
      return;
    }
    entry.chunks[c] = chunk;
  }

  memmove(&index.entries[1], &index.entries[0],
          index.count * sizeof(CacheEntry));
  index.entries[0] = entry;
  index.count++;
  write_index(&index);
}

bool cache_load(const char *key, int32_t *flags, char *buffer, size_t size) {
  CacheIndex index;
  read_index(&index);
  int i = find(&index, key_hash(key, i18n_language()));
  if (i < 0 || size == 0) {
    return false;
  }
  const CacheEntry *entry = &index.entries[i];
  size_t len = entry->size < size - 1 ? entry->size : size - 1;
  for (int c = 0; c < num_chunks(entry); c++) {
    size_t offset = c * CHUNK_SIZE;
    if (offset >= len) {
      break;
    }
    size_t chunk_len = len - offset < CHUNK_SIZE ? len - offset : CHUNK_SIZE;
    if (persist_read_data(PERSIST_KEY_CACHE_CHUNK + entry->chunks[c],
                          buffer + offset, chunk_len) < (int)chunk_len) {
      return false;
    }
  }
  buffer[len] = '\0';
  *flags = entry->flags;
  return true;
}

int32_t cache_version(const char *key) {
  CacheIndex index;
  read_index(&index);
  int i = find(&index, key_hash(key, i18n_language()));
  if (i < 0) {
    return 0;
  }
  const CacheEntry *entry = &index.entries[i];
  return (int32_t)((entry->data_hash ^ (uint8_t)entry->flags) * 16777619u);
}

void cache_remove(const char *key) {
  CacheIndex index;
  read_index(&index);
  // in every language, so a deleted memo does not come back after a
  // language change
  bool removed = false;
  for (Language language = LanguageEnglish; language < LanguageCount;
       language++) {
    int i = find(&index, key_hash(key, language));
    if (i >= 0) {
      remove_entry(&index, i);
      removed = true;
    }
  }
  if (removed) {
    write_index(&index);
  }
}

void cache_make_room(size_t others) {
  CacheIndex index;
  read_index(&index);
  uint8_t count = index.count;
  shrink(&index, 0, budget(others));
  if (index.count != count) {
    write_index(&index);
  }
}
