/* repl_app.h - Inline REPL TUI component: textinput only, no alt screen
 *
 * Inline mode renders in the primary terminal buffer — no viewport,
 * no borders. Output goes directly to stdout; the terminal's own
 * scrollback is the output history.
 */

#ifndef REPL_APP_H
#define REPL_APP_H

#include <stddef.h>

#include <boba/component.h>
#include <boba/components/list_popup.h>
#include <boba/components/textinput.h>

/* Forward declaration — full type in boba/runtime.h */
typedef struct TuiRuntime TuiRuntime;

/* Custom message types for Elm Architecture completion flow */
#define REPL_MSG_COMPLETIONS_READY (TUI_MSG_CUSTOM_BASE + 1)

/* Payload for REPL_MSG_COMPLETIONS_READY — owned by the message.
 * The update handler must free texts/prefix after processing. */
typedef struct
{
    char **texts; /* NULL-terminated array of completion strings (owned) */
    int count;
    int word_start;
    char *prefix; /* the prefix that was completed (owned) */
} ReplCompletionsData;

/* ReplApp configuration */
typedef struct
{
    int terminal_width;
    int terminal_height;
} ReplAppConfig;

/* ReplApp model — textinput + completion popup in inline mode */
typedef struct
{
    TuiModel base;
    TuiTextInput *textinput;
    TuiListPopup *popup;
    int terminal_width;
    int terminal_height;

    /* Callback: returns 1 if text is a complete form, 0 if incomplete.
     * Used to decide whether Enter evaluates or inserts a newline. */
    int (*is_complete)(const char *text);

    /* Callback: returns the number of spaces to indent the new line
     * after Enter is pressed on an incomplete form. Receives the text
     * after the newline was inserted. */
    int (*compute_indent)(const char *text);

    /* Callback: called when Ctrl-C is pressed to abort the current
     * edit and start a fresh prompt. */
    void (*on_break)(void);

    void (*on_submit)(char *text);

    /* Completion provider: fetches completions for a prefix.
     * Returns a NULL-terminated array of strings (caller frees via
     * free_completions). May return NULL. */
    char **(*on_tab_complete)(const char *prefix, int word_start,
                              const char *buffer, int cursor_pos);
    void (*free_completions)(char **completions);
} ReplAppModel;

/* TuiComponent interface */
TuiInitResult repl_app_init(void *config);
TuiUpdateResult repl_app_update(TuiModel *model, TuiMsg msg);
TuiView repl_app_view(const TuiModel *model, DynamicBuffer *out);
void repl_app_free(TuiModel *model);

/* Set the prompt string */
void repl_app_set_prompt(ReplAppModel *app, const char *prompt);

/* Set runtime handle — needed for posting completion messages back
 * to the update loop. Call after runtime creation. */
void repl_app_set_runtime(TuiRuntime *rt);

/* Get component interface for ReplApp */
const TuiComponent *repl_app_component(void);

#endif /* REPL_APP_H */
