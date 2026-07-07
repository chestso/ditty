/* repl_app.c - Inline REPL TUI component implementation
 *
 * Textinput + completion popup, inline rendering (no alt screen).
 * Output goes directly to stdout; terminal scrollback is the output history.
 *
 * Completion follows the Elm Architecture:
 *   Tab → textinput emits TUI_CMD_TAB_COMPLETE → bubbles to main.c
 *   main.c fetches completions → posts REPL_MSG_COMPLETIONS_READY
 *   repl_app_update receives the message → populates popup, shows it
 *   When popup visible: Tab/Up/Down navigate, Enter inserts, Esc dismisses
 */

#include "repl_app.h"
#include <boba/ansi_sequences.h>
#include <boba/cmd.h>
#include <boba/msg.h>
#include <boba/runtime.h>
#include <stdlib.h>
#include <string.h>

#define REPL_APP_TYPE_ID (TUI_COMPONENT_TYPE_BASE + 20)

/* Runtime handle — needed by repl_app_update to post completion messages.
 * Set by the caller after runtime creation. */
static TuiRuntime *s_runtime = NULL;

void repl_app_set_runtime(TuiRuntime *rt)
{
    s_runtime = rt;
}

TuiInitResult repl_app_init(void *config)
{
    const ReplAppConfig *cfg = (const ReplAppConfig *)config;

    ReplAppModel *app = (ReplAppModel *)malloc(sizeof(ReplAppModel));
    if (!app)
        return tui_init_result_none(NULL);

    memset(app, 0, sizeof(ReplAppModel));
    app->base.type = REPL_APP_TYPE_ID;

    app->terminal_width = cfg && cfg->terminal_width > 0 ? cfg->terminal_width : 80;
    app->terminal_height = cfg && cfg->terminal_height > 0 ? cfg->terminal_height : 24;

    /* Create textinput (user input) — multiline so Enter can insert newlines */
    TuiTextInputConfig textinput_cfg = { .multiline = 1 };
    app->textinput = tui_textinput_create(&textinput_cfg);
    if (!app->textinput) {
        free(app);
        return tui_init_result_none(NULL);
    }
    tui_textinput_set_prompt(app->textinput, ">>> ");
    tui_textinput_set_continuation_prompt(app->textinput, "... ");
    tui_textinput_set_terminal_width(app->textinput, app->terminal_width);
    tui_textinput_set_soft_wrap(app->textinput, 1);
    tui_textinput_set_history_size(app->textinput, 500);
    tui_textinput_set_word_chars(app->textinput,
                                 "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-*!?");

    /* Create completion popup */
    app->popup = tui_list_popup_create();
    if (!app->popup) {
        tui_textinput_free(app->textinput);
        free(app);
        return tui_init_result_none(NULL);
    }
    tui_list_popup_set_terminal_size(app->popup,
                                     app->terminal_width, app->terminal_height);
    tui_list_popup_set_title(app->popup, "completions");

    return tui_init_result_none((TuiModel *)app);
}

void repl_app_free(TuiModel *model)
{
    ReplAppModel *app = (ReplAppModel *)model;
    if (!app)
        return;

    if (app->popup)
        tui_list_popup_free(app->popup);
    if (app->textinput)
        tui_textinput_free(app->textinput);
    free(app);
}

/* Insert the selected completion and hide the popup */
static void insert_selected_and_hide(ReplAppModel *app)
{
    const char *selected = tui_list_popup_selected_text(app->popup);
    if (selected) {
        int word_start = tui_list_popup_word_start(app->popup);
        tui_textinput_insert_completion(app->textinput, word_start, selected);
    }
    tui_list_popup_hide(app->popup);
}

TuiUpdateResult repl_app_update(TuiModel *model, TuiMsg msg)
{
    ReplAppModel *app = (ReplAppModel *)model;
    if (!app)
        return tui_update_result_none();

    if (msg.type == TUI_MSG_WINDOW_SIZE) {
        app->terminal_width = msg.data.size.width;
        app->terminal_height = msg.data.size.height;
        tui_textinput_set_terminal_width(app->textinput, app->terminal_width);
        tui_list_popup_set_terminal_size(app->popup,
                                         app->terminal_width, app->terminal_height);
        return tui_update_result_none();
    }

    /* Handle interrupt (Ctrl+C) — abort current edit, dismiss popup */
    if (msg.type == TUI_MSG_INTERRUPT) {
        if (tui_list_popup_is_visible(app->popup))
            tui_list_popup_hide(app->popup);
        if (app->on_break)
            app->on_break();
        return tui_update_result_none();
    }

    /* Handle EOF (Ctrl+D) — quit on empty input, delete char otherwise */
    if (msg.type == TUI_MSG_EOF) {
        if (tui_list_popup_is_visible(app->popup)) {
            tui_list_popup_hide(app->popup);
            return tui_update_result_none();
        }
        if (tui_textinput_len(app->textinput) == 0)
            return tui_update_result(tui_cmd_quit());
        /* Non-empty: forward as Ctrl+D key to textinput for delete-char */
        return tui_textinput_update(app->textinput,
                                    tui_msg_key(TUI_KEY_NONE, 'd', TUI_MOD_CTRL));
    }

    /* Handle completions ready message — pure model update */
    if (msg.type == REPL_MSG_COMPLETIONS_READY) {
        ReplCompletionsData *data = (ReplCompletionsData *)msg.data.custom;
        int popup_was_visible = tui_list_popup_is_visible(app->popup);

        if (data->count == 0) {
            /* No completions — hide popup if visible */
            if (popup_was_visible)
                tui_list_popup_hide(app->popup);
        } else if (data->count == 1 && !popup_was_visible) {
            /* First-Tab single match — insert directly, no popup */
            tui_textinput_insert_completion(app->textinput,
                                            data->word_start, data->texts[0]);
        } else {
            /* Multiple matches OR live-filter refresh — update popup */
            tui_list_popup_set_items(app->popup,
                                     (const char *const *)data->texts,
                                     data->count);
            tui_list_popup_set_filter(app->popup, data->prefix);
            if (!popup_was_visible)
                tui_list_popup_show(app->popup, data->word_start);
            /* If already visible: items updated, selection resets to 0 */
        }

        /* Free the message payload (we own it now) */
        if (app->free_completions && data->texts)
            app->free_completions(data->texts);
        free(data->prefix);
        free(data);
        return tui_update_result_none();
    }

    if (msg.type == TUI_MSG_KEY_PRESS) {
        /* When popup is visible, intercept navigation keys */
        if (tui_list_popup_is_visible(app->popup)) {
            int key = msg.data.key.key;
            int mods = msg.data.key.mods;

            if (key == TUI_KEY_TAB && !(mods & TUI_MOD_SHIFT)) {
                tui_list_popup_move_down(app->popup);
                return tui_update_result_none();
            }
            if (key == TUI_KEY_TAB && (mods & TUI_MOD_SHIFT)) {
                tui_list_popup_move_up(app->popup);
                return tui_update_result_none();
            }
            if (key == TUI_KEY_ENTER) {
                insert_selected_and_hide(app);
                return tui_update_result_none();
            }
            if (key == TUI_KEY_ESCAPE) {
                tui_list_popup_hide(app->popup);
                return tui_update_result_none();
            }
            if (key == TUI_KEY_UP) {
                tui_list_popup_move_up(app->popup);
                return tui_update_result_none();
            }
            if (key == TUI_KEY_DOWN) {
                tui_list_popup_move_down(app->popup);
                return tui_update_result_none();
            }
            if (key == TUI_KEY_PAGE_UP) {
                tui_list_popup_move_page_up(app->popup);
                return tui_update_result_none();
            }
            if (key == TUI_KEY_PAGE_DOWN) {
                tui_list_popup_move_page_down(app->popup);
                return tui_update_result_none();
            }
            if (key == TUI_KEY_HOME) {
                tui_list_popup_move_top(app->popup);
                return tui_update_result_none();
            }
            if (key == TUI_KEY_END) {
                tui_list_popup_move_bottom(app->popup);
                return tui_update_result_none();
            }
            /* Ctrl+G dismisses */
            if (key == TUI_KEY_NONE && msg.data.key.rune == 'g' &&
                (mods & TUI_MOD_CTRL)) {
                tui_list_popup_hide(app->popup);
                return tui_update_result_none();
            }

            /* Edit keys (printable, backspace, etc.): forward to textinput,
             * then re-extract prefix and emit TAB_COMPLETE so main.c can
             * refresh the popup list. The popup stays visible. */
            TuiUpdateResult edit_r = tui_textinput_update(app->textinput, msg);

            int ws = 0;
            char *new_prefix = tui_textinput_word_at_cursor(app->textinput, &ws);
            if (!new_prefix || !*new_prefix) {
                /* Cursor left the word — hide popup */
                tui_list_popup_hide(app->popup);
                free(new_prefix);
                return edit_r;
            }

            TuiCmd *complete_cmd = tui_cmd_tab_complete(new_prefix, ws);
            if (edit_r.cmd)
                return tui_update_result(
                    tui_cmd_batch2(edit_r.cmd, complete_cmd));
            return tui_update_result(complete_cmd);
        }

        /* Intercept Enter: if the form is incomplete, insert a newline
         * instead of submitting. If complete and on_submit is set,
         * call on_submit instead of letting line_submit fire. */
        if (msg.data.key.key == TUI_KEY_ENTER &&
            !(msg.data.key.mods & TUI_MOD_SHIFT) && app->is_complete) {
            const char *text = tui_textinput_text(app->textinput);
            if (text && !app->is_complete(text)) {
                /* Incomplete — insert newline, then auto-indent */
                TuiUpdateResult r = tui_textinput_update(app->textinput,
                                                         tui_msg_key(TUI_KEY_ENTER, 0, TUI_MOD_SHIFT));
                if (r.cmd)
                    tui_cmd_free(r.cmd);
                /* Compute and insert auto-indent spaces */
                if (app->compute_indent) {
                    const char *after = tui_textinput_text(app->textinput);
                    int indent = app->compute_indent(after);
                    for (int i = 0; i < indent; i++)
                        tui_textinput_update(app->textinput, tui_msg_char(' ', 0));
                }
                return tui_update_result_none();
            }
            /* Complete — call on_submit with the text, then clear */
            if (app->on_submit) {
                char *saved = strdup(text ? text : "");
                tui_textinput_clear(app->textinput);
                app->on_submit(saved);
                return tui_update_result_none();
            }
        }

        /* Forward to textinput. If it emits TUI_CMD_TAB_COMPLETE,
         * let it bubble up to main.c's cmd handler. */
        return tui_textinput_update(app->textinput, msg);
    }

    return tui_update_result_none();
}

TuiView repl_app_view(const TuiModel *model, DynamicBuffer *out)
{
    const ReplAppModel *app = (const ReplAppModel *)model;
    if (!app || !out)
        return tui_view_default(out);

    /* Render textinput (prompt + input lines) */
    tui_textinput_view(app->textinput, out);

    /* Render popup below textinput if visible */
    if (tui_list_popup_is_visible(app->popup)) {
        dynamic_buffer_append_str(out, "\r\n");
        tui_list_popup_view(app->popup, out);
    }

    /* Declare inline mode + bracketed paste. No alt screen, no mouse. */
    TuiView v = tui_view_default(out);
    v.render_mode = TUI_RENDER_INLINE;
    v.bracketed_paste = 1;
    v.cursor = tui_textinput_cursor_pos(app->textinput);
    return v;
}

void repl_app_set_prompt(ReplAppModel *app, const char *prompt)
{
    if (app && app->textinput)
        tui_textinput_set_prompt(app->textinput, prompt);
}

const TuiComponent *repl_app_component(void)
{
    static const TuiComponent comp = {
        .init = repl_app_init,
        .update = repl_app_update,
        .view = repl_app_view,
        .free = repl_app_free,
    };
    return &comp;
}
