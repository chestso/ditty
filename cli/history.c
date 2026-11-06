/* history.c - REPL history persistence */

#include "history.h"
#include "../include/file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif

/* ---- Internal helpers (exposed for testing) ---- */

/* Escape a line for storage: \n -> \n, \\ -> \\ */
char *history_escape(const char *line)
{
    if (!line)
        return NULL;

    size_t len = strlen(line);
    /* Worst case: every char needs escaping */
    char *result = malloc(len * 2 + 1);
    if (!result)
        return NULL;

    char *out = result;
    for (size_t i = 0; i < len; i++) {
        if (line[i] == '\n') {
            *out++ = '\\';
            *out++ = 'n';
        } else if (line[i] == '\\') {
            *out++ = '\\';
            *out++ = '\\';
        } else {
            *out++ = line[i];
        }
    }
    *out = '\0';
    return result;
}

/* Unescape a line from storage: \n -> newline, \\ -> \ */
char *history_unescape(const char *escaped)
{
    if (!escaped)
        return NULL;

    size_t len = strlen(escaped);
    char *result = malloc(len + 1);
    if (!result)
        return NULL;

    char *out = result;
    for (size_t i = 0; i < len; i++) {
        if (escaped[i] == '\\' && i + 1 < len) {
            if (escaped[i + 1] == 'n') {
                *out++ = '\n';
                i++;
            } else if (escaped[i + 1] == '\\') {
                *out++ = '\\';
                i++;
            } else {
                /* Unknown escape: keep the backslash */
                *out++ = '\\';
            }
        } else {
            *out++ = escaped[i];
        }
    }
    *out = '\0';
    return result;
}

/* Get the history file path. Returns a static buffer. */
const char *history_get_path(void)
{
    static char path[4096];

#ifdef _WIN32
    /* Windows: %LOCALAPPDATA%\ditty\history */
    char appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appdata))) {
        snprintf(path, sizeof(path), "%s\\ditty\\history", appdata);
    } else {
        /* Fallback to USERPROFILE */
        const char *home = getenv("USERPROFILE");
        if (home) {
            snprintf(path, sizeof(path), "%s\\AppData\\Local\\ditty\\history", home);
        } else {
            snprintf(path, sizeof(path), "ditty\\history");
        }
    }
#else
    /* POSIX: $XDG_STATE_HOME/ditty/history or ~/.local/state/ditty/history */
    const char *xdg_state = getenv("XDG_STATE_HOME");
    if (xdg_state && xdg_state[0] != '\0') {
        snprintf(path, sizeof(path), "%s/ditty/history", xdg_state);
    } else {
        const char *home = getenv("HOME");
        if (!home) {
            struct passwd *pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : "/";
        }
        snprintf(path, sizeof(path), "%s/.local/state/ditty/history", home);
    }
#endif

    return path;
}

/* ---- Public API ----
 *
 * The history itself lives in the textinput: boba owns the ring, including
 * the dedup, cap and empty-line rules, and hands it back through
 * tui_textinput_history_count() / _history_entry(). ditty owns only what is
 * policy rather than state — the file's path, its escape format, and the
 * order it writes in. */

void history_load(TuiTextInput *input)
{
    if (!input)
        return;

    const char *path = history_get_path();
    FILE *f = file_open(path, "r");
    if (!f)
        return;

    char line[8192];
    while (fgets(line, sizeof(line), f)) {
        /* Strip trailing newline (the file's line separator, not escaped \n) */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
#ifdef _WIN32
            /* Also strip \r for CRLF files */
            if (len > 1 && line[len - 2] == '\r') {
                line[len - 2] = '\0';
            }
#endif
        }

        /* Unescape and add to history */
        char *unescaped = history_unescape(line);
        if (unescaped) {
            tui_textinput_history_add(input, unescaped);
            free(unescaped);
        }
    }

    fclose(f);
}

void history_save(const TuiTextInput *input)
{
    if (!input || tui_textinput_history_count(input) == 0)
        return;

    const char *path = history_get_path();

    /* Ensure directory exists */
    char dir[4096];
    file_dirname(path, dir, sizeof(dir));
    if (!file_exists(dir)) {
        if (file_mkdir_p(dir) != 0) {
            return; /* Silent fail */
        }
    }

    FILE *f = file_open(path, "w");
    if (!f)
        return;

    /* The ring is newest-first (index 0 is what Up reaches first) and the
     * file is oldest-first, so it reads chronologically and loading it back
     * in order restores the same ring. */
    int count = tui_textinput_history_count(input);
    for (int i = count - 1; i >= 0; i--) {
        const char *entry = tui_textinput_history_entry(input, i);
        char *escaped = history_escape(entry);
        if (escaped) {
            fprintf(f, "%s\n", escaped);
            free(escaped);
        }
    }

    fclose(f);
}
