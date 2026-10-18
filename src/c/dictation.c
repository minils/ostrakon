#include "dictation.h"

#include "i18n.h"
#include "toast.h"
#include "ui.h"

static bool is_cancel(DictationSessionStatus status) {
  return status == DictationSessionStatusFailureTranscriptionRejected ||
         status == DictationSessionStatusFailureSystemAborted;
}

static void session_finished(DictationSession *session,
                             DictationSessionStatus status, char *text,
                             void *context) {
  Dictation *dictation = context;
  if (status == DictationSessionStatusSuccess) {
    if (!ui_is_blank(text)) {
      dictation->handler(text);
    }
  } else if (!is_cancel(status)) {
    toast_show(tr("Dictation Failed"), false);
  }
}

void dictation_start(Dictation *dictation) {
  if (!dictation->session) {
    dictation->session =
        dictation_session_create(0, session_finished, dictation);
    if (!dictation->session) {
      toast_show(tr("Dictation Failed"), false);
      return;
    }
    dictation_session_enable_confirmation(dictation->session,
                                          dictation->confirm);
  }
  dictation_session_start(dictation->session);
}

void dictation_release(Dictation *dictation) {
  if (dictation->session) {
    dictation_session_destroy(dictation->session);
    dictation->session = NULL;
  }
}

bool dictation_join(char *buffer, size_t size, const char *text) {
  size_t used = strlen(buffer);
  const char *format = used > 0 ? " %s" : "%s";
  int needed = snprintf(buffer + used, size - used, format, text);
  if (needed < 0 || (size_t)needed >= size - used) {
    buffer[used] = '\0';
    return false;
  }
  return true;
}
