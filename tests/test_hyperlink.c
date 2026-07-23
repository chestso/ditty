/* test_hyperlink.c - OSC 8 hyperlink detection and formatting tests */

#include "flare_testkit.h"
#include "lisp.h"
#include <ditty/highlight.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
/* MinGW UCRT64 lacks setenv/unsetenv; implement them with _putenv_s.
 * Note: Windows CRT cannot represent an empty environment variable;
 * _putenv_s(name, "") deletes the variable. */
static int setenv(const char *name, const char *value, int overwrite)
{
    if (!overwrite && getenv(name))
        return 0;
    return _putenv_s(name, value);
}
static int unsetenv(const char *name)
{
    return _putenv_s(name, "");
}
#endif

static Environment *env;

/* Helper: setenv wrapper that returns old value for restoration */
static char *save_env(const char *name)
{
    char *val = getenv(name);
    return val ? strdup(val) : NULL;
}

static void restore_env(const char *name, char *saved)
{
    if (saved) {
        setenv(name, saved, 1);
        free(saved);
    } else {
        unsetenv(name);
    }
}

/* Save and clear all terminal-identifying env vars so tests start
 * from a known baseline.  Call restore_all_terminal_envs() afterwards. */
typedef struct
{
    char *force, *no_color, *force_color, *ci, *teamcity, *term;
    char *tmux, *sty, *wt_session, *kitty, *domterm, *ghostty;
    char *alacritty_log, *alacritty_id, *alacritty_socket;
    char *terminal_emu, *konsole, *tp, *tpv, *vte;
} TerminalEnvs;

static void save_all_terminal_envs(TerminalEnvs *e)
{
    e->force = save_env("FORCE_HYPERLINK");
    e->no_color = save_env("NO_COLOR");
    e->force_color = save_env("FORCE_COLOR");
    e->ci = save_env("CI");
    e->teamcity = save_env("TEAMCITY_VERSION");
    e->term = save_env("TERM");
    e->tmux = save_env("TMUX");
    e->sty = save_env("STY");
    e->wt_session = save_env("WT_SESSION");
    e->kitty = save_env("KITTY_WINDOW_ID");
    e->domterm = save_env("DOMTERM");
    e->ghostty = save_env("GHOSTTY_RESOURCES_DIR");
    e->alacritty_log = save_env("ALACRITTY_LOG");
    e->alacritty_id = save_env("ALACRITTY_WINDOW_ID");
    e->alacritty_socket = save_env("ALACRITTY_SOCKET");
    e->terminal_emu = save_env("TERMINAL_EMULATOR");
    e->konsole = save_env("KONSOLE_VERSION");
    e->tp = save_env("TERM_PROGRAM");
    e->tpv = save_env("TERM_PROGRAM_VERSION");
    e->vte = save_env("VTE_VERSION");

    unsetenv("FORCE_HYPERLINK");
    unsetenv("NO_COLOR");
    unsetenv("FORCE_COLOR");
    unsetenv("CI");
    unsetenv("TEAMCITY_VERSION");
    unsetenv("TMUX");
    unsetenv("STY");
    unsetenv("WT_SESSION");
    unsetenv("KITTY_WINDOW_ID");
    unsetenv("DOMTERM");
    unsetenv("GHOSTTY_RESOURCES_DIR");
    unsetenv("ALACRITTY_LOG");
    unsetenv("ALACRITTY_WINDOW_ID");
    unsetenv("ALACRITTY_SOCKET");
    unsetenv("TERMINAL_EMULATOR");
    unsetenv("KONSOLE_VERSION");
    unsetenv("TERM_PROGRAM");
    unsetenv("TERM_PROGRAM_VERSION");
    unsetenv("VTE_VERSION");
}

static void restore_all_terminal_envs(TerminalEnvs *e)
{
    restore_env("FORCE_HYPERLINK", e->force);
    restore_env("NO_COLOR", e->no_color);
    restore_env("FORCE_COLOR", e->force_color);
    restore_env("CI", e->ci);
    restore_env("TEAMCITY_VERSION", e->teamcity);
    restore_env("TERM", e->term);
    restore_env("TMUX", e->tmux);
    restore_env("STY", e->sty);
    restore_env("WT_SESSION", e->wt_session);
    restore_env("KITTY_WINDOW_ID", e->kitty);
    restore_env("DOMTERM", e->domterm);
    restore_env("GHOSTTY_RESOURCES_DIR", e->ghostty);
    restore_env("ALACRITTY_LOG", e->alacritty_log);
    restore_env("ALACRITTY_WINDOW_ID", e->alacritty_id);
    restore_env("ALACRITTY_SOCKET", e->alacritty_socket);
    restore_env("TERMINAL_EMULATOR", e->terminal_emu);
    restore_env("KONSOLE_VERSION", e->konsole);
    restore_env("TERM_PROGRAM", e->tp);
    restore_env("TERM_PROGRAM_VERSION", e->tpv);
    restore_env("VTE_VERSION", e->vte);
}

/* ----- Detection tests ------------------------------------------------- */

static void test_non_tty_disables_hyperlinks(void)
{
    /* isatty() guard: without FORCE_HYPERLINK and on a non-TTY stdout,
     * the public function must return 0. */
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    ASSERT_TRUE(!flare_terminal_supports_hyperlinks());
    restore_all_terminal_envs(&e);
}

static void test_force_hyperlink_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("FORCE_HYPERLINK", "1", 1);
    ASSERT_TRUE(flare_terminal_supports_hyperlinks());
    restore_all_terminal_envs(&e);
}

static void test_force_hyperlink_zero_disables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("FORCE_HYPERLINK", "0", 1);
    ASSERT_TRUE(!flare_terminal_supports_hyperlinks());
    restore_all_terminal_envs(&e);
}

static void test_force_hyperlink_empty_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    int rc = setenv("FORCE_HYPERLINK", "", 1);
    fprintf(stderr, "[test_force_hyperlink_empty_enables] setenv() returned %d, getenv()=%p\n",
            rc, (void *)getenv("FORCE_HYPERLINK"));
    fflush(stderr);
#ifdef _WIN32
    /* Windows CRT cannot represent an empty environment variable; it
     * deletes the variable instead. Skip the empty-value check on Windows
     * and just verify a non-empty value still works. */
    setenv("FORCE_HYPERLINK", "1", 1);
#endif
    ASSERT_TRUE(flare_terminal_supports_hyperlinks());
    restore_all_terminal_envs(&e);
}

static void test_no_color_disables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("NO_COLOR", "1", 1);
    ASSERT_TRUE(!flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_ci_disables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("CI", "true", 1);
    ASSERT_TRUE(!flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_term_dumb_disables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("TERM", "dumb", 1);
    ASSERT_TRUE(!flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_tmux_disables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("TMUX", "/tmp/tmux-1000/default,1234,0", 1);
    ASSERT_TRUE(!flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_screen_disables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("STY", "12345.pts-0.hostname", 1);
    ASSERT_TRUE(!flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_kitty_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("KITTY_WINDOW_ID", "1", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_term_xterm_kitty_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("TERM", "xterm-kitty", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_ghostty_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("GHOSTTY_RESOURCES_DIR", "/opt/ghostty", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_windows_terminal_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("WT_SESSION", "abc123", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_domterm_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("DOMTERM", "1", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_term_program_ghostty_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("TERM_PROGRAM", "ghostty", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_iterm2_new_enough_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("TERM_PROGRAM", "iTerm.app", 1);
    setenv("TERM_PROGRAM_VERSION", "3.4.20", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_iterm2_too_old_disables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("TERM_PROGRAM", "iTerm.app", 1);
    setenv("TERM_PROGRAM_VERSION", "2.9.0", 1);
    ASSERT_TRUE(!flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_vte_new_enough_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("VTE_VERSION", "6800", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_vte_5000_segfault_excluded(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("VTE_VERSION", "5000", 1);
    ASSERT_TRUE(!flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_vte_too_old_disables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("VTE_VERSION", "4600", 1);
    ASSERT_TRUE(!flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_konsole_new_enough_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("KONSOLE_VERSION", "230805", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_konsole_too_old_disables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("KONSOLE_VERSION", "200402", 1);
    ASSERT_TRUE(!flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_wezterm_new_enough_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("TERM_PROGRAM", "WezTerm", 1);
    setenv("TERM_PROGRAM_VERSION", "20220807", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_vscode_new_enough_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("TERM_PROGRAM", "vscode", 1);
    setenv("TERM_PROGRAM_VERSION", "1.72.0", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_alacritty_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("ALACRITTY_WINDOW_ID", "12345", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_jetbrains_jediterm_enables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("TERMINAL_EMULATOR", "JetBrains-JediTerm", 1);
    ASSERT_TRUE(flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

static void test_unknown_terminal_disables(void)
{
    TerminalEnvs e;
    save_all_terminal_envs(&e);
    setenv("TERM", "xterm-256color", 1);
    ASSERT_TRUE(!flare_terminal_hyperlinks_env());
    restore_all_terminal_envs(&e);
}

/* ----- Formatting tests ------------------------------------------------ */

static void test_hyperlink_format_inline_link(void)
{
    /* "[text](https://example.com)" should produce OSC 8 hyperlink */
    const char *input = "[text](https://example.com)";
    FlareLexer *lexer = flare_lexer_commonmark(env);
    ASSERT_NOT_NULL(lexer);

    size_t count;
    FlareToken *tokens = flare_lex(lexer, input, strlen(input), &count);
    ASSERT_NOT_NULL(tokens);
    ASSERT_TRUE(count > 0);

    /* First token should be the inline link */
    ASSERT_EQ(tokens[0].type, HL_MARKUP_INLINE_LINK);

    FlareStyle *style = flare_style_monokai();
    char *output = flare_format_terminal_ex(tokens, count, input, style,
                                            BFLARE_COLOR_TRUECOLOR, 1);
    ASSERT_NOT_NULL(output);

    /* Should contain OSC 8 open sequence */
    ASSERT_TRUE(strstr(output, "\x1b]8;;https://example.com\x07") != NULL);
    /* Should contain visible text */
    ASSERT_TRUE(strstr(output, "text") != NULL);
    /* Should contain OSC 8 close sequence */
    ASSERT_TRUE(strstr(output, "\x1b]8;;\x07") != NULL);

    free(output);
    flare_style_free(style);
    free(tokens);
    flare_lexer_free(lexer);
}

static void test_hyperlink_format_autolink(void)
{
    /* "<https://example.com>" should produce OSC 8 hyperlink */
    const char *input = "<https://example.com>";
    FlareLexer *lexer = flare_lexer_commonmark(env);
    ASSERT_NOT_NULL(lexer);

    size_t count;
    FlareToken *tokens = flare_lex(lexer, input, strlen(input), &count);
    ASSERT_NOT_NULL(tokens);

    FlareStyle *style = flare_style_monokai();
    char *output = flare_format_terminal_ex(tokens, count, input, style,
                                            BFLARE_COLOR_TRUECOLOR, 1);
    ASSERT_NOT_NULL(output);

    /* Should contain OSC 8 open with URI */
    ASSERT_TRUE(strstr(output, "\x1b]8;;https://example.com\x07") != NULL);
    /* URI is also the display text for autolinks */
    ASSERT_TRUE(strstr(output, "https://example.com") != NULL);
    /* Should contain close */
    ASSERT_TRUE(strstr(output, "\x1b]8;;\x07") != NULL);

    free(output);
    flare_style_free(style);
    free(tokens);
    flare_lexer_free(lexer);
}

static void test_hyperlink_disabled_no_osc8(void)
{
    /* Same input, but hyperlink disabled */
    const char *input = "[text](https://example.com)";
    FlareLexer *lexer = flare_lexer_commonmark(env);
    ASSERT_NOT_NULL(lexer);

    size_t count;
    FlareToken *tokens = flare_lex(lexer, input, strlen(input), &count);
    ASSERT_NOT_NULL(tokens);

    FlareStyle *style = flare_style_monokai();
    char *output = flare_format_terminal_ex(tokens, count, input, style,
                                            BFLARE_COLOR_TRUECOLOR, 0);
    ASSERT_NOT_NULL(output);

    /* Should NOT contain OSC 8 sequences */
    ASSERT_TRUE(strstr(output, "\x1b]8") == NULL);
    /* Should still contain the visible text */
    ASSERT_TRUE(strstr(output, "text") != NULL);

    free(output);
    flare_style_free(style);
    free(tokens);
    flare_lexer_free(lexer);
}

static void test_hyperlink_any_scheme_emitted(void)
{
    /* All URI schemes are emitted; the terminal decides what to open */
    const char *input = "[click](javascript:alert(1))";
    FlareLexer *lexer = flare_lexer_commonmark(env);
    ASSERT_NOT_NULL(lexer);

    size_t count;
    FlareToken *tokens = flare_lex(lexer, input, strlen(input), &count);
    ASSERT_NOT_NULL(tokens);

    FlareStyle *style = flare_style_monokai();
    char *output = flare_format_terminal_ex(tokens, count, input, style,
                                            BFLARE_COLOR_TRUECOLOR, 1);
    ASSERT_NOT_NULL(output);

    /* Should contain OSC 8 with the URI as-is */
    ASSERT_TRUE(strstr(output, "\x1b]8;;javascript:alert(1)\x07") != NULL);
    /* Should still render the text */
    ASSERT_TRUE(strstr(output, "click") != NULL);

    free(output);
    flare_style_free(style);
    free(tokens);
    flare_lexer_free(lexer);
}

static void test_image_no_hyperlink(void)
{
    /* Images shouldn't get hyperlinks */
    const char *input = "![alt](https://example.com/img.png)";
    FlareLexer *lexer = flare_lexer_commonmark(env);
    ASSERT_NOT_NULL(lexer);

    size_t count;
    FlareToken *tokens = flare_lex(lexer, input, strlen(input), &count);
    ASSERT_NOT_NULL(tokens);

    FlareStyle *style = flare_style_monokai();
    char *output = flare_format_terminal_ex(tokens, count, input, style,
                                            BFLARE_COLOR_TRUECOLOR, 1);
    ASSERT_NOT_NULL(output);

    /* Should NOT contain OSC 8 for images */
    ASSERT_TRUE(strstr(output, "\x1b]8") == NULL);

    free(output);
    flare_style_free(style);
    free(tokens);
    flare_lexer_free(lexer);
}

int main(void)
{
    env = lisp_init();

    printf("OSC 8 hyperlink detection tests:\n");

    /* Detection tests */
    RUN_TEST(test_non_tty_disables_hyperlinks);
    RUN_TEST(test_force_hyperlink_enables);
    RUN_TEST(test_force_hyperlink_zero_disables);
    RUN_TEST(test_force_hyperlink_empty_enables);
    RUN_TEST(test_no_color_disables);
    RUN_TEST(test_ci_disables);
    RUN_TEST(test_term_dumb_disables);
    RUN_TEST(test_tmux_disables);
    RUN_TEST(test_screen_disables);
    RUN_TEST(test_kitty_enables);
    RUN_TEST(test_term_xterm_kitty_enables);
    RUN_TEST(test_ghostty_enables);
    RUN_TEST(test_windows_terminal_enables);
    RUN_TEST(test_domterm_enables);
    RUN_TEST(test_term_program_ghostty_enables);
    RUN_TEST(test_iterm2_new_enough_enables);
    RUN_TEST(test_iterm2_too_old_disables);
    RUN_TEST(test_vte_new_enough_enables);
    RUN_TEST(test_vte_5000_segfault_excluded);
    RUN_TEST(test_vte_too_old_disables);
    RUN_TEST(test_konsole_new_enough_enables);
    RUN_TEST(test_konsole_too_old_disables);
    RUN_TEST(test_wezterm_new_enough_enables);
    RUN_TEST(test_vscode_new_enough_enables);
    RUN_TEST(test_alacritty_enables);
    RUN_TEST(test_jetbrains_jediterm_enables);
    RUN_TEST(test_unknown_terminal_disables);

    /* Formatting tests */
    printf("\nOSC 8 hyperlink formatting tests:\n");
    RUN_TEST(test_hyperlink_format_inline_link);
    RUN_TEST(test_hyperlink_format_autolink);
    RUN_TEST(test_hyperlink_disabled_no_osc8);
    RUN_TEST(test_hyperlink_any_scheme_emitted);
    RUN_TEST(test_image_no_hyperlink);

    TEST_SUMMARY();
}
