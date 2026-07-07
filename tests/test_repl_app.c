/* test_repl_app.c - Tests for inline REPL app component
 *
 * Tests the on_submit/is_complete callback pattern, Enter interception
 * for complete vs incomplete forms, and cmd ownership/free safety.
 */

#include "../cli/repl_app.h"
#include "../include/lisp.h"
#include <assert.h>
#include <boba/ansi_sequences.h>
#include <boba/cmd.h>
#include <boba/components/textinput.h>
#include <boba/dynamic_buffer.h>
#include <boba/msg.h>
#include <boba/runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define RUN_TEST(fn)                 \
    do {                             \
        tests_run++;                 \
        fn();                        \
        tests_passed++;              \
        printf("  PASS: %s\n", #fn); \
    } while (0)

#define ASSERT_TRUE(cond, msg)                     \
    do {                                           \
        if (!(cond)) {                             \
            fprintf(stderr, "  FAIL: %s:%d: %s\n", \
                    __FILE__, __LINE__, msg);      \
            abort();                               \
        }                                          \
    } while (0)

/* --- Helpers --- */

static void send_string(TuiTextInput *input, const char *s)
{
    for (const char *p = s; *p; p++)
        tui_textinput_update(input, tui_msg_char((uint32_t)*p, 0));
}

/* is_complete callback using lisp_read */
static int is_form_complete(const char *text)
{
    const char *ptr = text;
    LispObject *expr = lisp_read(&ptr);
    if (expr == NULL)
        return 1;
    if (LISP_TYPE(expr) == LISP_ERROR &&
        LISP_ERROR_TYPE(expr) == sym_unclosed_input)
        return 0;
    return 1;
}

/* --- Tests --- */

/* #3: on_submit callback is invoked with the submitted text.
 * When Enter is pressed on a complete form, the on_submit callback
 * receives the text and the textinput is cleared. */
static char *g_submitted_text = NULL;
static int g_submit_count = 0;

static void on_submit_capture(char *text)
{
    g_submitted_text = text;
    g_submit_count++;
}

static void test_on_submit_invoked_with_text(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ASSERT_TRUE(ir.model != NULL, "init should succeed");
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->on_submit = on_submit_capture;

    /* Type a complete form */
    send_string(app->textinput, "(+ 1 2)");
    ASSERT_TRUE(strcmp(tui_textinput_text(app->textinput), "(+ 1 2)") == 0,
                "text should be typed");

    /* Press Enter — should trigger on_submit */
    g_submitted_text = NULL;
    g_submit_count = 0;
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(g_submit_count == 1, "on_submit should be called once");
    ASSERT_TRUE(g_submitted_text != NULL, "submitted text should not be NULL");
    ASSERT_TRUE(strcmp(g_submitted_text, "(+ 1 2)") == 0,
                "submitted text should match input");
    ASSERT_TRUE(tui_textinput_len(app->textinput) == 0,
                "textinput should be cleared after submit");

    free(g_submitted_text);
    g_submitted_text = NULL;
    repl_app_free((TuiModel *)app);
}

/* #4a: Enter on incomplete form inserts newline, does NOT call on_submit */
static void test_enter_incomplete_inserts_newline(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->on_submit = on_submit_capture;

    /* Type an incomplete form */
    send_string(app->textinput, "(define");

    g_submit_count = 0;
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(g_submit_count == 0, "on_submit should NOT be called");
    ASSERT_TRUE(tui_textinput_line_count(app->textinput) == 2,
                "should have 2 lines after newline insertion");

    repl_app_free((TuiModel *)app);
}

/* #4b: Enter on complete form calls on_submit, no newline */
static void test_enter_complete_calls_submit(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->on_submit = on_submit_capture;

    send_string(app->textinput, "42");

    g_submit_count = 0;
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(g_submit_count == 1, "on_submit should be called");
    ASSERT_TRUE(tui_textinput_line_count(app->textinput) == 1,
                "should still be 1 line (no newline inserted)");

    free(g_submitted_text);
    g_submitted_text = NULL;
    repl_app_free((TuiModel *)app);
}

/* Auto-indent: Enter on incomplete form inserts newline + indent spaces */
static int compute_indent_2x(const char *text)
{
    int depth = 0;
    for (const char *p = text; *p; p++) {
        if (*p == '(')
            depth++;
        else if (*p == ')')
            depth--;
    }
    return depth > 0 ? depth * 2 : 0;
}

static void test_enter_incomplete_auto_indents(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->compute_indent = compute_indent_2x;

    /* Type "((42" — 2 unclosed parens */
    send_string(app->textinput, "((42");

    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    /* After Enter: text should be "((42\n  " (newline + 4 spaces) */
    const char *text = tui_textinput_text(app->textinput);
    ASSERT_TRUE(strstr(text, "\n") != NULL, "should have newline");
    /* The line after \n should start with 4 spaces (2 parens * 2) */
    const char *nl = strchr(text, '\n');
    ASSERT_TRUE(nl != NULL, "newline found");
    ASSERT_TRUE(nl[1] == ' ' && nl[2] == ' ' && nl[3] == ' ' && nl[4] == ' ',
                "should have 4 spaces after newline");

    repl_app_free((TuiModel *)app);
}

/* Ctrl-C aborts the current edit and clears the textinput */
static int g_break_count = 0;
static void on_break_test(void)
{
    g_break_count++;
}

static void test_ctrl_c_aborts_edit(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->on_break = on_break_test;

    send_string(app->textinput, "(+ 1");
    ASSERT_TRUE(tui_textinput_len(app->textinput) > 0, "should have text");

    g_break_count = 0;
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_interrupt());
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(g_break_count == 1, "on_break should be called");
    /* on_break clears the textinput */
    tui_textinput_clear(app->textinput);
    ASSERT_TRUE(tui_textinput_len(app->textinput) == 0,
                "textinput should be empty after break");

    repl_app_free((TuiModel *)app);
}

/* #5: on_submit path doesn't double-free submitted text.
 * The on_submit callback receives a malloc'd string and is responsible
 * for freeing it. The runtime must not also free it. This test verifies
 * that freeing in the callback doesn't cause a crash (ASan would catch
 * a double-free or use-after-free). */
static void on_submit_free(char *text)
{
    free(text); /* callback frees the text */
}

static void test_on_submit_no_double_free(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->on_submit = on_submit_free;

    send_string(app->textinput, "(+ 1 2)");

    /* This should not crash — on_submit frees the text, and no
     * other code should touch it afterward */
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    /* If we get here without ASan error, the test passes */
    ASSERT_TRUE(1, "no double-free crash");

    repl_app_free((TuiModel *)app);
}

/* #9: After on_submit, the textinput view renders an empty prompt.
 * The view should show ">>> " with no input text. This verifies
 * that the input is cleared and the prompt is ready for next input. */
static void test_view_empty_after_submit(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->is_complete = is_form_complete;
    app->on_submit = on_submit_free;

    send_string(app->textinput, "(+ 1 2)");

    /* Submit */
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    /* Render view — should show empty prompt */
    DynamicBuffer *buf = dynamic_buffer_create(0);
    repl_app_view((const TuiModel *)app, buf);
    const char *data = dynamic_buffer_data(buf);

    /* Should contain ">>> " prompt */
    ASSERT_TRUE(strstr(data, ">>> ") != NULL, "view should show prompt");
    /* Should NOT contain the submitted text */
    ASSERT_TRUE(strstr(data, "(+ 1 2)") == NULL,
                "view should not contain submitted text");

    dynamic_buffer_destroy(buf);
    repl_app_free((TuiModel *)app);
}

/* ======================================================================
 * Completion popup integration tests
 * ====================================================================== */

/* Completion provider that returns a fixed set of strings */
static char **mock_completions(const char *prefix, int word_start,
                               const char *buffer, int cursor_pos)
{
    (void)word_start;
    (void)buffer;
    (void)cursor_pos;
    /* Return completions matching "str" prefix for testing */
    if (strcmp(prefix, "str") == 0) {
        char **comps = malloc(4 * sizeof(char *));
        comps[0] = strdup("string-length");
        comps[1] = strdup("string-append");
        comps[2] = strdup("string-contains");
        comps[3] = NULL;
        return comps;
    }
    if (strcmp(prefix, "st") == 0) {
        char **comps = malloc(4 * sizeof(char *));
        comps[0] = strdup("string-length");
        comps[1] = strdup("string-append");
        comps[2] = strdup("starts-with");
        comps[3] = NULL;
        return comps;
    }
    return NULL;
}

static void mock_free_completions(char **comps)
{
    if (!comps)
        return;
    for (int i = 0; comps[i]; i++)
        free(comps[i]);
    free(comps);
}

static void free_completions_data(ReplCompletionsData *data)
{
    if (!data)
        return;
    if (data->texts) {
        for (int i = 0; i < data->count; i++)
            free(data->texts[i]);
        free(data->texts);
    }
    free(data->prefix);
    free(data);
}

/* Tab triggers TUI_CMD_TAB_COMPLETE from textinput — verify the cmd
 * bubbles up so main.c can fetch completions and post them back. */
static void test_tab_emits_tab_complete_cmd(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    send_string(app->textinput, "str");

    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_TAB, 0, 0));
    ASSERT_TRUE(r.cmd != NULL, "Tab should emit a command");
    ASSERT_TRUE(r.cmd->type == TUI_CMD_TAB_COMPLETE,
                "command should be TUI_CMD_TAB_COMPLETE");
    ASSERT_TRUE(strcmp(r.cmd->payload.tab_complete.prefix, "str") == 0,
                "prefix should be 'str'");

    tui_cmd_free(r.cmd);
    repl_app_free((TuiModel *)app);
}

/* REPL_MSG_COMPLETIONS_READY with single match inserts directly, no popup */
static void test_single_completion_inserts_no_popup(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->free_completions = mock_free_completions;

    /* Simulate a completions-ready message with 1 match */
    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(2 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = NULL;
    data->count = 1;
    data->word_start = 0;
    data->prefix = strdup("str");

    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(!tui_list_popup_is_visible(app->popup),
                "popup should NOT be visible for single match");
    ASSERT_TRUE(strcmp(tui_textinput_text(app->textinput), "string-length") == 0,
                "text should be the single completion");

    repl_app_free((TuiModel *)app);
}

/* REPL_MSG_COMPLETIONS_READY with multiple matches shows popup */
static void test_multiple_completions_shows_popup(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->free_completions = mock_free_completions;

    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(4 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = strdup("string-append");
    data->texts[2] = strdup("string-contains");
    data->texts[3] = NULL;
    data->count = 3;
    data->word_start = 0;
    data->prefix = strdup("str");

    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(tui_list_popup_is_visible(app->popup),
                "popup should be visible for multiple matches");
    ASSERT_TRUE(tui_list_popup_selected_index(app->popup) == 0,
                "first item should be selected");
    ASSERT_TRUE(strcmp(tui_list_popup_selected_text(app->popup), "string-length") == 0,
                "selected text should be first completion");

    repl_app_free((TuiModel *)app);
}

/* When popup is visible, Tab cycles selection down */
static void test_popup_tab_cycles_down(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->free_completions = mock_free_completions;

    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(4 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = strdup("string-append");
    data->texts[2] = strdup("string-contains");
    data->texts[3] = NULL;
    data->count = 3;
    data->word_start = 0;
    data->prefix = strdup("str");

    TuiUpdateResult r0 = repl_app_update((TuiModel *)app,
                                         tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r0.cmd)
        tui_cmd_free(r0.cmd);

    /* Tab should cycle down, not emit TAB_COMPLETE */
    TuiUpdateResult r1 = repl_app_update((TuiModel *)app,
                                         tui_msg_key(TUI_KEY_TAB, 0, 0));
    ASSERT_TRUE(r1.cmd == NULL, "Tab should not emit cmd when popup visible");
    ASSERT_TRUE(tui_list_popup_selected_index(app->popup) == 1,
                "selection should be at index 1");

    /* Tab again → index 2 */
    TuiUpdateResult r2 = repl_app_update((TuiModel *)app,
                                         tui_msg_key(TUI_KEY_TAB, 0, 0));
    if (r2.cmd)
        tui_cmd_free(r2.cmd);
    ASSERT_TRUE(tui_list_popup_selected_index(app->popup) == 2,
                "selection should be at index 2");

    /* Tab again → wraps to 0 */
    TuiUpdateResult r3 = repl_app_update((TuiModel *)app,
                                         tui_msg_key(TUI_KEY_TAB, 0, 0));
    if (r3.cmd)
        tui_cmd_free(r3.cmd);
    ASSERT_TRUE(tui_list_popup_selected_index(app->popup) == 0,
                "selection should wrap to 0");

    repl_app_free((TuiModel *)app);
}

/* Enter on popup inserts selected completion and hides popup */
static void test_popup_enter_inserts_and_hides(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->free_completions = mock_free_completions;

    /* Set up initial text */
    send_string(app->textinput, "str");

    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(4 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = strdup("string-append");
    data->texts[2] = strdup("string-contains");
    data->texts[3] = NULL;
    data->count = 3;
    data->word_start = 0;
    data->prefix = strdup("str");

    TuiUpdateResult r0 = repl_app_update((TuiModel *)app,
                                         tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r0.cmd)
        tui_cmd_free(r0.cmd);

    /* Move to second item */
    TuiUpdateResult r1 = repl_app_update((TuiModel *)app,
                                         tui_msg_key(TUI_KEY_TAB, 0, 0));
    if (r1.cmd)
        tui_cmd_free(r1.cmd);

    /* Enter should insert "string-append" and hide popup */
    TuiUpdateResult r2 = repl_app_update((TuiModel *)app,
                                         tui_msg_key(TUI_KEY_ENTER, 0, 0));
    if (r2.cmd)
        tui_cmd_free(r2.cmd);

    ASSERT_TRUE(!tui_list_popup_is_visible(app->popup),
                "popup should be hidden after Enter");
    ASSERT_TRUE(strcmp(tui_textinput_text(app->textinput), "string-append") == 0,
                "text should be the selected completion");

    repl_app_free((TuiModel *)app);
}

/* Escape dismisses popup without inserting */
static void test_popup_escape_dismisses(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->free_completions = mock_free_completions;

    send_string(app->textinput, "str");

    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(3 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = strdup("string-append");
    data->texts[2] = NULL;
    data->count = 2;
    data->word_start = 0;
    data->prefix = strdup("str");

    TuiUpdateResult r0 = repl_app_update((TuiModel *)app,
                                         tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r0.cmd)
        tui_cmd_free(r0.cmd);

    ASSERT_TRUE(tui_list_popup_is_visible(app->popup), "popup should be visible");

    /* Escape should dismiss */
    TuiUpdateResult r1 = repl_app_update((TuiModel *)app,
                                         tui_msg_key(TUI_KEY_ESCAPE, 0, 0));
    if (r1.cmd)
        tui_cmd_free(r1.cmd);

    ASSERT_TRUE(!tui_list_popup_is_visible(app->popup),
                "popup should be hidden after Escape");
    /* Text should remain "str" (not inserted) */
    ASSERT_TRUE(strcmp(tui_textinput_text(app->textinput), "str") == 0,
                "text should remain unchanged after Escape");

    repl_app_free((TuiModel *)app);
}

/* Ctrl+C dismisses popup and calls on_break */
static void test_popup_ctrl_c_dismisses_and_breaks(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->free_completions = mock_free_completions;
    app->on_break = on_break_test;

    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(3 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = strdup("string-append");
    data->texts[2] = NULL;
    data->count = 2;
    data->word_start = 0;
    data->prefix = strdup("str");

    TuiUpdateResult r0 = repl_app_update((TuiModel *)app,
                                         tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r0.cmd)
        tui_cmd_free(r0.cmd);

    ASSERT_TRUE(tui_list_popup_is_visible(app->popup), "popup should be visible");

    g_break_count = 0;
    TuiUpdateResult r1 = repl_app_update((TuiModel *)app,
                                         tui_msg_interrupt());
    if (r1.cmd)
        tui_cmd_free(r1.cmd);

    ASSERT_TRUE(!tui_list_popup_is_visible(app->popup),
                "popup should be hidden after Ctrl+C");
    ASSERT_TRUE(g_break_count == 1, "on_break should be called");

    repl_app_free((TuiModel *)app);
}

/* Arrow Up/Down navigate popup */
static void test_popup_arrow_keys_navigate(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->free_completions = mock_free_completions;

    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(4 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = strdup("string-append");
    data->texts[2] = strdup("string-contains");
    data->texts[3] = NULL;
    data->count = 3;
    data->word_start = 0;
    data->prefix = strdup("str");

    TuiUpdateResult r0 = repl_app_update((TuiModel *)app,
                                         tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r0.cmd)
        tui_cmd_free(r0.cmd);

    /* Down → index 1 */
    TuiUpdateResult r1 = repl_app_update((TuiModel *)app,
                                         tui_msg_key(TUI_KEY_DOWN, 0, 0));
    if (r1.cmd)
        tui_cmd_free(r1.cmd);
    ASSERT_TRUE(tui_list_popup_selected_index(app->popup) == 1,
                "Down should move to index 1");

    /* Down → index 2 */
    TuiUpdateResult r2 = repl_app_update((TuiModel *)app,
                                         tui_msg_key(TUI_KEY_DOWN, 0, 0));
    if (r2.cmd)
        tui_cmd_free(r2.cmd);
    ASSERT_TRUE(tui_list_popup_selected_index(app->popup) == 2,
                "Down should move to index 2");

    /* Up → index 1 */
    TuiUpdateResult r3 = repl_app_update((TuiModel *)app,
                                         tui_msg_key(TUI_KEY_UP, 0, 0));
    if (r3.cmd)
        tui_cmd_free(r3.cmd);
    ASSERT_TRUE(tui_list_popup_selected_index(app->popup) == 1,
                "Up should move to index 1");

    repl_app_free((TuiModel *)app);
}

/* Popup view renders border and items */
static void test_popup_view_renders_content(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    app->free_completions = mock_free_completions;

    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(4 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = strdup("string-append");
    data->texts[2] = strdup("string-contains");
    data->texts[3] = NULL;
    data->count = 3;
    data->word_start = 0;
    data->prefix = strdup("str");

    TuiUpdateResult r0 = repl_app_update((TuiModel *)app,
                                         tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r0.cmd)
        tui_cmd_free(r0.cmd);

    /* Render view */
    DynamicBuffer *buf = dynamic_buffer_create(0);
    repl_app_view((const TuiModel *)app, buf);
    const char *view_data = dynamic_buffer_data(buf);

    /* Should contain popup items */
    ASSERT_TRUE(strstr(view_data, "string-length") != NULL,
                "view should contain first completion");
    ASSERT_TRUE(strstr(view_data, "string-append") != NULL,
                "view should contain second completion");
    /* Should contain SGR styling for the selected row */
    ASSERT_TRUE(strstr(view_data, "\033[") != NULL,
                "view should contain ANSI styling");

    dynamic_buffer_destroy(buf);
    repl_app_free((TuiModel *)app);
}

/* View without popup doesn't render popup content */
static void test_view_no_popup_when_hidden(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    DynamicBuffer *buf = dynamic_buffer_create(0);
    repl_app_view((const TuiModel *)app, buf);
    const char *view_data = dynamic_buffer_data(buf);

    /* Should NOT contain popup border */
    ASSERT_TRUE(strstr(view_data, "completions") == NULL,
                "view should not contain popup title when hidden");

    dynamic_buffer_destroy(buf);
    repl_app_free((TuiModel *)app);
}

/* ======================================================================
 * Live filtering tests — typing while popup visible updates the list
 * instead of dismissing it.
 * ====================================================================== */

/* Helper: show popup with 3 items so we can test live filtering */
static void setup_popup_with_items(ReplAppModel *app)
{
    app->free_completions = mock_free_completions;

    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(4 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = strdup("string-append");
    data->texts[2] = strdup("string-contains");
    data->texts[3] = NULL;
    data->count = 3;
    data->word_start = 0;
    data->prefix = strdup("str");

    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r.cmd)
        tui_cmd_free(r.cmd);
}

/* Typing a char while popup is visible emits TAB_COMPLETE, doesn't dismiss */
static void test_live_filter_typing_emits_tab_complete(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    send_string(app->textinput, "str");
    setup_popup_with_items(app);
    ASSERT_TRUE(tui_list_popup_is_visible(app->popup), "popup should be visible");

    /* Type 'i' — should forward to textinput AND emit TAB_COMPLETE */
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_char('i', 0));
    ASSERT_TRUE(r.cmd != NULL, "typing should emit TAB_COMPLETE cmd");
    ASSERT_TRUE(r.cmd->type == TUI_CMD_TAB_COMPLETE,
                "cmd should be TAB_COMPLETE");
    ASSERT_TRUE(strcmp(r.cmd->payload.tab_complete.prefix, "stri") == 0,
                "prefix should be 'stri'");
    ASSERT_TRUE(tui_list_popup_is_visible(app->popup),
                "popup should still be visible (not dismissed)");
    ASSERT_TRUE(strcmp(tui_textinput_text(app->textinput), "stri") == 0,
                "textinput should have 'stri'");

    tui_cmd_free(r.cmd);
    repl_app_free((TuiModel *)app);
}

/* Backspace while popup is visible emits TAB_COMPLETE with shorter prefix */
static void test_live_filter_backspace_emits_tab_complete(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    send_string(app->textinput, "str");
    setup_popup_with_items(app);
    ASSERT_TRUE(tui_list_popup_is_visible(app->popup), "popup should be visible");

    /* Backspace — should emit TAB_COMPLETE with prefix "st" */
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_BACKSPACE, 0, 0));
    ASSERT_TRUE(r.cmd != NULL, "backspace should emit TAB_COMPLETE cmd");
    ASSERT_TRUE(strcmp(r.cmd->payload.tab_complete.prefix, "st") == 0,
                "prefix should be 'st'");
    ASSERT_TRUE(tui_list_popup_is_visible(app->popup),
                "popup should still be visible");

    tui_cmd_free(r.cmd);
    repl_app_free((TuiModel *)app);
}

/* Backspace to empty word returns NULL prefix — popup hides */
static void test_live_filter_empty_prefix_hides_popup(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    send_string(app->textinput, "s");
    setup_popup_with_items(app);
    ASSERT_TRUE(tui_list_popup_is_visible(app->popup), "popup should be visible");

    /* Backspace to empty — no word at cursor, popup should hide */
    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_key(TUI_KEY_BACKSPACE, 0, 0));
    /* No TAB_COMPLETE cmd (word_at_cursor returns NULL) */
    ASSERT_TRUE(r.cmd == NULL, "no cmd when cursor leaves word");
    ASSERT_TRUE(!tui_list_popup_is_visible(app->popup),
                "popup should be hidden when prefix is empty");
    ASSERT_TRUE(strcmp(tui_textinput_text(app->textinput), "") == 0,
                "textinput should be empty");

    if (r.cmd)
        tui_cmd_free(r.cmd);
    repl_app_free((TuiModel *)app);
}

/* COMPLETIONS_READY with popup already visible updates items (not show) */
static void test_completions_ready_updates_existing_popup(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    send_string(app->textinput, "str");
    setup_popup_with_items(app);
    ASSERT_TRUE(tui_list_popup_is_visible(app->popup), "popup visible");
    ASSERT_TRUE(tui_list_popup_selected_index(app->popup) == 0,
                "first item selected");

    /* Move selection to index 2 */
    tui_list_popup_move_down(app->popup);
    tui_list_popup_move_down(app->popup);
    ASSERT_TRUE(tui_list_popup_selected_index(app->popup) == 2,
                "selection at index 2");

    /* Simulate a refresh: COMPLETIONS_READY arrives while popup visible */
    app->free_completions = mock_free_completions;
    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(3 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = strdup("string-append");
    data->texts[2] = NULL;
    data->count = 2;
    data->word_start = 0;
    data->prefix = strdup("stri");

    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(tui_list_popup_is_visible(app->popup),
                "popup should still be visible after refresh");
    /* Selection should reset to 0 (new list) */
    ASSERT_TRUE(tui_list_popup_selected_index(app->popup) == 0,
                "selection should reset to 0 on refresh");
    ASSERT_TRUE(strcmp(tui_list_popup_selected_text(app->popup), "string-length") == 0,
                "first item should be 'string-length'");

    repl_app_free((TuiModel *)app);
}

/* COMPLETIONS_READY with 0 matches hides popup */
static void test_completions_ready_zero_matches_hides_popup(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    send_string(app->textinput, "str");
    setup_popup_with_items(app);
    ASSERT_TRUE(tui_list_popup_is_visible(app->popup), "popup visible");

    /* Simulate 0 matches */
    app->free_completions = mock_free_completions;
    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = NULL;
    data->count = 0;
    data->word_start = 0;
    data->prefix = strdup("strxyz");

    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    ASSERT_TRUE(!tui_list_popup_is_visible(app->popup),
                "popup should hide on 0 matches");

    repl_app_free((TuiModel *)app);
}

/* COMPLETIONS_READY with 1 match during refresh shows in popup (no auto-insert) */
static void test_completions_ready_single_match_refresh_keeps_popup(void)
{
    ReplAppConfig cfg = { .terminal_width = 80, .terminal_height = 24 };
    TuiInitResult ir = repl_app_init(&cfg);
    ReplAppModel *app = (ReplAppModel *)ir.model;

    send_string(app->textinput, "str");
    setup_popup_with_items(app);
    ASSERT_TRUE(tui_list_popup_is_visible(app->popup), "popup visible");

    /* Simulate 1 match during refresh */
    app->free_completions = mock_free_completions;
    ReplCompletionsData *data = calloc(1, sizeof(ReplCompletionsData));
    data->texts = malloc(2 * sizeof(char *));
    data->texts[0] = strdup("string-length");
    data->texts[1] = NULL;
    data->count = 1;
    data->word_start = 0;
    data->prefix = strdup("string-l");

    TuiUpdateResult r = repl_app_update((TuiModel *)app,
                                        tui_msg_custom(REPL_MSG_COMPLETIONS_READY, data));
    if (r.cmd)
        tui_cmd_free(r.cmd);

    /* During refresh, 1 match shows in popup — no auto-insert */
    ASSERT_TRUE(tui_list_popup_is_visible(app->popup),
                "popup should stay visible for single match during refresh");
    ASSERT_TRUE(strcmp(tui_list_popup_selected_text(app->popup), "string-length") == 0,
                "popup should show the single match");
    /* Text should NOT be auto-inserted */
    ASSERT_TRUE(strcmp(tui_textinput_text(app->textinput), "str") == 0,
                "text should not change during refresh single match");

    repl_app_free((TuiModel *)app);
}

int main(void)
{
    lisp_init();

    printf("repl_app tests:\n");

    RUN_TEST(test_on_submit_invoked_with_text);
    RUN_TEST(test_enter_incomplete_inserts_newline);
    RUN_TEST(test_enter_complete_calls_submit);
    RUN_TEST(test_enter_incomplete_auto_indents);
    RUN_TEST(test_ctrl_c_aborts_edit);
    RUN_TEST(test_on_submit_no_double_free);
    RUN_TEST(test_view_empty_after_submit);

    /* Completion popup tests */
    RUN_TEST(test_tab_emits_tab_complete_cmd);
    RUN_TEST(test_single_completion_inserts_no_popup);
    RUN_TEST(test_multiple_completions_shows_popup);
    RUN_TEST(test_popup_tab_cycles_down);
    RUN_TEST(test_popup_enter_inserts_and_hides);
    RUN_TEST(test_popup_escape_dismisses);
    RUN_TEST(test_popup_ctrl_c_dismisses_and_breaks);
    RUN_TEST(test_popup_arrow_keys_navigate);
    RUN_TEST(test_popup_view_renders_content);
    RUN_TEST(test_view_no_popup_when_hidden);

    /* Live filtering tests */
    RUN_TEST(test_live_filter_typing_emits_tab_complete);
    RUN_TEST(test_live_filter_backspace_emits_tab_complete);
    RUN_TEST(test_live_filter_empty_prefix_hides_popup);
    RUN_TEST(test_completions_ready_updates_existing_popup);
    RUN_TEST(test_completions_ready_zero_matches_hides_popup);
    RUN_TEST(test_completions_ready_single_match_refresh_keeps_popup);

    lisp_cleanup();

    printf("\n%d/%d tests passed.\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
