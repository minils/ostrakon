#include "search.h"

#include "comm.h"
#include "dictation.h"
#include "i18n.h"
#include "memo_list.h"
#include "ui.h"

// The phone matches words, so trailing punctuation would get in the way
static void query_dictated(const char *text) {
  char query[COMM_TITLE_SIZE];
  ui_copy_truncated(query, text, sizeof(query));
  for (size_t len = strlen(query); len > 0; len--) {
    if (!strchr(".,!? ", query[len - 1])) {
      break;
    }
    query[len - 1] = '\0';
  }
  memo_list_push_search(query);
}

// a short query needs no confirmation screen
static Dictation s_dictation = {.handler = query_dictated, .confirm = false};

void search_start(void) { dictation_start(&s_dictation); }
