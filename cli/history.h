/* history.h - REPL history persistence */

#ifndef DITTY_HISTORY_H
#define DITTY_HISTORY_H

#include <boba/components/textinput.h>

/* Entries the REPL asks the textinput to keep (its history ring) — the size
 * of the saved file follows from it, since saving writes that ring. */
#define HISTORY_MAX_ENTRIES 500

/* Load history from disk into the textinput's history ring.
 * Safe to call if the history file doesn't exist (no-op). */
void history_load(TuiTextInput *input);

/* Save the textinput's history ring to disk, oldest line first.
 * Creates the history directory if needed. Silent no-op on failure. */
void history_save(const TuiTextInput *input);

#endif /* DITTY_HISTORY_H */
