/* flare_terminal.c - Terminal capability detection for OSC 8 hyperlinks */

#include "../include/ditty/highlight.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Parse major.minor from version string */
static void parse_version(const char *v, int *major, int *minor)
{
    *major = 0;
    *minor = 0;
    if (!v || !v[0])
        return;

    *major = atoi(v);
    const char *dot = strchr(v, '.');
    if (dot)
        *minor = atoi(dot + 1);
}

/* Detect OSC 8 hyperlink support from environment variables only.
 * Does not check isatty() — the public function adds that guard. */
int flare_terminal_hyperlinks_env(void)
{
    /* 1. FORCE_HYPERLINK override */
    const char *force = getenv("FORCE_HYPERLINK");
    if (force != NULL) {
        if (force[0] == '\0')
            return 1; /* empty = enable */
        if (strcmp(force, "0") == 0)
            return 0; /* "0" = disable */
        return 1;     /* any other value = enable */
    }

    /* 2. Color/CI opt-outs */
    if (getenv("NO_COLOR") != NULL)
        return 0;

    const char *fc = getenv("FORCE_COLOR");
    if (fc && (strcmp(fc, "0") == 0 || strcmp(fc, "false") == 0))
        return 0;

    if (getenv("CI") != NULL)
        return 0;

    if (getenv("TEAMCITY_VERSION") != NULL)
        return 0;

    const char *term = getenv("TERM");
    if (term && strcmp(term, "dumb") == 0)
        return 0;

    /* 3. Multiplexers: disable by default (host identity hidden) */
    if (getenv("TMUX") != NULL)
        return 0;

    if (getenv("STY") != NULL)
        return 0;

    /* 4. Terminals with unconditional support (env var presence = support) */
    if (getenv("WT_SESSION") != NULL)
        return 1; /* Windows Terminal */

    if (getenv("KITTY_WINDOW_ID") != NULL)
        return 1; /* Kitty */

    if (getenv("DOMTERM") != NULL)
        return 1; /* DomTerm */

    if (getenv("GHOSTTY_RESOURCES_DIR") != NULL)
        return 1; /* Ghostty */

    if (getenv("ALACRITTY_LOG") != NULL || getenv("ALACRITTY_WINDOW_ID") != NULL ||
        getenv("ALACRITTY_SOCKET") != NULL)
        return 1; /* Alacritty */

    const char *te = getenv("TERMINAL_EMULATOR");
    if (te && strcmp(te, "JetBrains-JediTerm") == 0)
        return 1;

    if (term) {
        if (strcmp(term, "xterm-kitty") == 0)
            return 1;
        if (strcmp(term, "xterm-ghostty") == 0)
            return 1;
        if (strcmp(term, "alacritty") == 0)
            return 1;
    }

    /* 5. Konsole: KONSOLE_VERSION >= 210400 (packed int) */
    const char *kv = getenv("KONSOLE_VERSION");
    if (kv) {
        long v = atol(kv);
        if (v >= 210400)
            return 1;
    }

    /* 6. TERM_PROGRAM + version-gated terminals */
    const char *tp = getenv("TERM_PROGRAM");
    const char *tpv = getenv("TERM_PROGRAM_VERSION");

    if (tp) {
        if (strcmp(tp, "iTerm.app") == 0) {
            /* iTerm2 >= 3.1 */
            if (tpv) {
                int major = 0, minor = 0;
                parse_version(tpv, &major, &minor);
                if (major > 3 || (major == 3 && minor >= 1))
                    return 1;
            }
            return 0;
        }

        if (strcmp(tp, "WezTerm") == 0) {
            /* WezTerm >= 20200620 (date-based version) */
            if (tpv) {
                int v = atoi(tpv);
                if (v >= 20200620)
                    return 1;
            }
            return 0;
        }

        if (strcmp(tp, "vscode") == 0) {
            /* VS Code >= 1.72 */
            if (tpv) {
                int major = 0, minor = 0;
                parse_version(tpv, &major, &minor);
                if (major > 1 || (major == 1 && minor >= 72))
                    return 1;
            }
            return 0;
        }

        if (strcmp(tp, "ghostty") == 0)
            return 1;

        if (strcmp(tp, "mintty") == 0) {
            /* mintty >= 3.3 */
            if (tpv) {
                int major = 0, minor = 0;
                parse_version(tpv, &major, &minor);
                if (major > 3 || (major == 3 && minor >= 3))
                    return 1;
            }
            return 0;
        }
    }

    /* 7. VTE_VERSION: >= 5000 (0.50+), exclude exactly 5000 (segfault bug) */
    const char *vte = getenv("VTE_VERSION");
    if (vte) {
        int v = atoi(vte);
        if (v == 5000)
            return 0; /* 0.50.0 segfaults */
        if (v >= 5000)
            return 1; /* >= 0.50 */
        return 0;
    }

    /* 8. Unknown terminal: default to safe (no hyperlinks) */
    return 0;
}

/* Detect OSC 8 hyperlink support via environment variable heuristics.
 * Checks isatty() first, then delegates to env-var-only detection. */
int flare_terminal_supports_hyperlinks(void)
{
    /* Non-TTY: never emit escape sequences to files/pipes.
     * FORCE_HYPERLINK override bypasses the isatty check. */
    const char *force = getenv("FORCE_HYPERLINK");
    if (force == NULL && !isatty(STDOUT_FILENO))
        return 0;

    return flare_terminal_hyperlinks_env();
}
