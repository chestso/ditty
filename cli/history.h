/* history.h - REPL history persistence */

#ifndef DITTY_HISTORY_H
#define DITTY_HISTORY_H

#include <boba/components/textinput.h>

/* Load history from disk into the textinput's history buffer.
 * Safe to call if the history file doesn't exist (no-op). */
void history_load(TuiTextInput *input);

/* Save history from the textinput to disk.
 * Creates the history directory if needed. Silent no-op on failure. */
void history_save(TuiTextInput *input);

#endif /* DITTY_HISTORY_H */
