#include "builtins_internal.h"
#include <unistd.h>

static LispObject *g_recursive_kw = NULL;
static LispObject *g_directory_kw = NULL;

/* Get user's home directory path (cross-platform)
 * Unix/Linux/macOS: $HOME
 * Windows: %USERPROFILE% or %HOMEDRIVE%%HOMEPATH%
 * Returns: String with home directory or NIL if not found
 */
static LispObject *builtin_home_directory(LispObject *args, Environment *env)
{
    (void)args; /* Takes no arguments */
    (void)env;

    const char *home = NULL;

#if defined(_WIN32) || defined(_WIN64)
    /* Windows: Try USERPROFILE first */
    home = getenv("USERPROFILE");

    /* Fallback: HOMEDRIVE + HOMEPATH */
    if (home == NULL) {
        const char *homedrive = getenv("HOMEDRIVE");
        const char *homepath = getenv("HOMEPATH");

        if (homedrive != NULL && homepath != NULL) {
            size_t len = strlen(homedrive) + strlen(homepath) + 1;
            char *combined = GC_malloc(len);
            snprintf(combined, len, "%s%s", homedrive, homepath);
            home = combined;
        }
    }
#else
    /* Unix/Linux/macOS: Use HOME */
    home = getenv("HOME");
#endif

    if (home == NULL) {
        return NIL; /* No home directory found */
    }

    return lisp_make_string(home);
}

/* Return the current working directory.
 * Racket style: (current-directory)
 * Unix/Linux/macOS: getcwd()
 * Windows: GetCurrentDirectory()
 * Returns: String with the current working directory path
 */
static LispObject *builtin_current_directory(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

    char buf[4096];
#if defined(_WIN32) || defined(_WIN64)
    DWORD len = GetCurrentDirectoryA(sizeof(buf), buf);
    if (len == 0 || len >= sizeof(buf))
        return lisp_make_error("current-directory: cannot determine current directory");
#else
    if (getcwd(buf, sizeof(buf)) == NULL)
        return lisp_make_error("current-directory: cannot determine current directory");
#endif

    return lisp_make_string(buf);
}

/* Expand ~/ in file paths to home directory (cross-platform)
 * Takes: String (file path)
 * Returns: String (expanded path) or original if no ~/ prefix
 * Example: (expand-path "~/config.lisp") => "/home/user/config.lisp"
 */
static LispObject *builtin_expand_path(LispObject *args, Environment *env)
{
    CHECK_ARGS_1("expand-path");

    LispObject *path_obj = lisp_car(args);
    if (LISP_TYPE(path_obj) != LISP_STRING) {
        return lisp_make_error("expand-path requires a string argument");
    }

    const char *path = LISP_STR_VAL(path_obj);

    /* Check if path starts with ~/ */
    if (path[0] != '~' || (path[1] != '/' && path[1] != '\\' && path[1] != '\0')) {
        /* Not a ~/ path - return original */
        return path_obj;
    }

    /* Get home directory */
    LispObject *home_obj = builtin_home_directory(NIL, env);
    if (home_obj == NIL || LISP_TYPE(home_obj) != LISP_STRING) {
        /* No home directory - return error */
        return lisp_make_error("expand-path: cannot determine home directory");
    }

    const char *home = LISP_STR_VAL(home_obj);

    /* Calculate expanded path length */
    /* If path is just "~", use home directory directly */
    if (path[1] == '\0') {
        return home_obj;
    }

    /* Skip ~/ or ~\ */
    const char *rest = path + 2;

    /* Build expanded path: home + / + rest */
    size_t home_len = strlen(home);
    size_t rest_len = strlen(rest);
    size_t total_len = home_len + 1 + rest_len + 1;

    char *expanded = GC_malloc(total_len);

#if defined(_WIN32) || defined(_WIN64)
    /* Windows: Use backslash separator */
    snprintf(expanded, total_len, "%s\\%s", home, rest);
#else
    /* Unix: Use forward slash separator */
    snprintf(expanded, total_len, "%s/%s", home, rest);
#endif

    return lisp_make_string(expanded);
}

/* Read an environment variable.
 * Takes: String (variable name)
 * Returns: String (value) or nil if not set
 */
static LispObject *builtin_getenv(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("getenv requires 1 argument");

    LispObject *name = lisp_car(args);
    if (LISP_TYPE(name) != LISP_STRING)
        return lisp_make_error("getenv requires a string argument");

    const char *value = getenv(LISP_STR_VAL(name));
    if (!value)
        return NIL;

    return lisp_make_string(value);
}

/* Return platform-specific user data directory for an application.
 * Takes: String (app name)
 * Returns: String (path, not created) or error
 * Unix: $XDG_DATA_HOME/app or ~/.local/share/app
 * Windows: %LOCALAPPDATA%\app or %APPDATA%\app (or XDG_DATA_HOME if set)
 */
static LispObject *builtin_data_directory(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("data-directory requires 1 argument");

    LispObject *app = lisp_car(args);
    if (LISP_TYPE(app) != LISP_STRING)
        return lisp_make_error("data-directory requires a string argument");

    const char *app_name = LISP_STR_VAL(app);
    char path[PATH_MAX];

#if defined(_WIN32) || defined(_WIN64)
    const char *xdg_data = getenv("XDG_DATA_HOME");
    if (xdg_data && xdg_data[0]) {
        snprintf(path, sizeof(path), "%s\\%s", xdg_data, app_name);
    } else {
        const char *dir = getenv("LOCALAPPDATA");
        if (!dir)
            dir = getenv("APPDATA");
        if (!dir)
            return lisp_make_error("data-directory: cannot determine data directory");
        snprintf(path, sizeof(path), "%s\\%s", dir, app_name);
    }
#else
    const char *dir = getenv("XDG_DATA_HOME");
    if (dir && dir[0]) {
        snprintf(path, sizeof(path), "%s/%s", dir, app_name);
    } else {
        const char *home = getenv("HOME");
        if (!home)
            return lisp_make_error("data-directory: cannot determine home directory");
        snprintf(path, sizeof(path), "%s/.local/share/%s", home, app_name);
    }
#endif

    return lisp_make_string(path);
}

/* Return platform-specific user config directory for an application.
 * Takes: String (app name)
 * Returns: String (path, not created) or error
 * Unix: $XDG_CONFIG_HOME/app or ~/.config/app
 * Windows: %APPDATA%\app (or XDG_CONFIG_HOME if set)
 */
static LispObject *builtin_config_directory(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("config-directory requires 1 argument");

    LispObject *app = lisp_car(args);
    if (LISP_TYPE(app) != LISP_STRING)
        return lisp_make_error("config-directory requires a string argument");

    const char *app_name = LISP_STR_VAL(app);
    char path[PATH_MAX];

#if defined(_WIN32) || defined(_WIN64)
    const char *xdg = getenv("XDG_CONFIG_HOME");
    if (xdg && xdg[0]) {
        snprintf(path, sizeof(path), "%s\\%s", xdg, app_name);
    } else {
        const char *dir = getenv("APPDATA");
        if (!dir)
            return lisp_make_error("config-directory: cannot determine config directory");
        snprintf(path, sizeof(path), "%s\\%s", dir, app_name);
    }
#else
    const char *dir = getenv("XDG_CONFIG_HOME");
    if (dir && dir[0]) {
        snprintf(path, sizeof(path), "%s/%s", dir, app_name);
    } else {
        const char *home = getenv("HOME");
        if (!home)
            return lisp_make_error("config-directory: cannot determine home directory");
        snprintf(path, sizeof(path), "%s/.config/%s", home, app_name);
    }
#endif

    return lisp_make_string(path);
}

/* Check if a file or directory exists.
 * Takes: String (path)
 * Returns: #t or nil
 */
static LispObject *builtin_file_exists_question(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("file-exists? requires 1 argument");

    LispObject *path = lisp_car(args);
    if (LISP_TYPE(path) != LISP_STRING)
        return lisp_make_error("file-exists? requires a string argument");

    return file_exists(LISP_STR_VAL(path)) ? LISP_TRUE : NIL;
}

/* Create directory and all parents (mkdir -p).
 * Takes: String (path)
 * Returns: #t or error
 */
static LispObject *builtin_mkdir(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("mkdir requires 1 argument");

    LispObject *path = lisp_car(args);
    if (LISP_TYPE(path) != LISP_STRING)
        return lisp_make_error("mkdir requires a string argument");

    if (file_exists(LISP_STR_VAL(path)))
        return LISP_TRUE;

    if (file_mkdir_p(LISP_STR_VAL(path)) != 0) {
        char errbuf[256];
        snprintf(errbuf, sizeof(errbuf), "mkdir: %s: %s", LISP_STR_VAL(path),
                 strerror(errno));
        return lisp_make_error(errbuf);
    }

    return LISP_TRUE;
}

/* Delete a directory.
 * (delete-directory path)            — delete empty directory only
 * (delete-directory path :recursive) — delete directory and all contents
 * Like Emacs Lisp's delete-directory with :recursive keyword. */
static LispObject *builtin_delete_directory(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("delete-directory requires at least 1 argument");

    LispObject *path = lisp_car(args);
    if (LISP_TYPE(path) != LISP_STRING)
        return lisp_make_error("delete-directory requires a string path");

    int recursive = 0;
    if (g_recursive_kw == NULL)
        g_recursive_kw = lisp_make_keyword(":recursive");
    LispObject *rest = lisp_cdr(args);
    while (rest != NIL) {
        LispObject *kw = lisp_car(rest);
        if (kw == g_recursive_kw) {
            recursive = 1;
            break;
        }
        rest = lisp_cdr(rest);
    }

    if (!file_is_directory(LISP_STR_VAL(path))) {
        char errbuf[512];
        snprintf(errbuf, sizeof(errbuf),
                 "delete-directory: '%s' is not a directory",
                 LISP_STR_VAL(path));
        return lisp_make_error(errbuf);
    }

    if (file_remove_directory(LISP_STR_VAL(path), recursive) != 0) {
        char errbuf[512];
        snprintf(errbuf, sizeof(errbuf),
                 "delete-directory: failed to delete '%s': %s",
                 LISP_STR_VAL(path), strerror(errno));
        return lisp_make_error(errbuf);
    }

    return NIL;
}

static LispObject *builtin_file_is_directory_question(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("file-is-directory?");

    LispObject *path = lisp_car(args);
    if (LISP_TYPE(path) != LISP_STRING)
        return lisp_make_error("file-is-directory? requires a string path");

    return file_is_directory(LISP_STR_VAL(path)) ? LISP_TRUE : NIL;
}

/* Return a symbol identifying the operating system family.
 * Emacs Lisp style: 'gnu/linux, 'windows-nt, 'darwin, 'msys, 'cygwin, etc.
 * Takes no arguments.
 * Returns: Symbol
 */
static LispObject *builtin_system_type(LispObject *args, Environment *env)
{
    (void)args;
    (void)env;

#if defined(_WIN32) || defined(_WIN64)
    return lisp_make_symbol("windows-nt");
#elif defined(__CYGWIN__)
    /* MSYS2 (including UCRT64/MINGW64 shells) defines __CYGWIN__ and
     * uname -s returns "MINGW64_NT-..." or "MSYS_NT-...".  MSYS2 is a
     * Cygwin derivative, so we report it as 'msys rather than
     * 'cygwin — the distinction matters for tests that need to know
     * the filesystem is case-insensitive. */
    return lisp_make_symbol("msys");
#elif defined(__APPLE__)
    return lisp_make_symbol("darwin");
#elif defined(__linux__)
    return lisp_make_symbol("gnu/linux");
#elif defined(__FreeBSD__)
    return lisp_make_symbol("freebsd");
#elif defined(__NetBSD__)
    return lisp_make_symbol("netbsd");
#elif defined(__OpenBSD__)
    return lisp_make_symbol("openbsd");
#else
    return lisp_make_symbol("unknown");
#endif
}

/* Return the system temporary file directory.
 * Emacs Lisp style: (temporary-file-directory)
 * Unix: $TMPDIR or /tmp
 * Windows: GetTempPath() (checks %TEMP%, %TMP%, %USERPROFILE%)
 * Returns: String with temp directory path
 */
static LispObject *builtin_temporary_file_directory(LispObject *args,
                                                    Environment *env)
{
    (void)args;
    (void)env;

    char buf[4096];
    if (file_temp_directory(buf, sizeof(buf)) != 0)
        return lisp_make_error("temporary-file-directory: cannot determine temp directory");

    return lisp_make_string(buf);
}

/* Create a unique temporary file or directory.
 * Emacs Lisp style: (make-temp-file prefix &optional :directory)
 * Creates a temp file by default, or a temp directory with :directory keyword.
 * Returns: String with the created path
 */
static LispObject *builtin_make_temp_file(LispObject *args, Environment *env)
{
    (void)env;
    if (args == NIL)
        return lisp_make_error("make-temp-file requires at least 1 argument");

    LispObject *prefix_obj = lisp_car(args);
    if (LISP_TYPE(prefix_obj) != LISP_STRING)
        return lisp_make_error("make-temp-file requires a string prefix");

    int make_dir = 0;
    if (g_directory_kw == NULL)
        g_directory_kw = lisp_make_keyword(":directory");
    LispObject *rest = lisp_cdr(args);
    while (rest != NIL) {
        if (lisp_car(rest) == g_directory_kw) {
            make_dir = 1;
            break;
        }
        rest = lisp_cdr(rest);
    }

    char buf[8192];
    const char *prefix = LISP_STR_VAL(prefix_obj);
    int rc;

    if (make_dir)
        rc = file_make_temp_dir(prefix, buf, sizeof(buf));
    else
        rc = file_make_temp_file(prefix, buf, sizeof(buf));

    if (rc != 0)
        return lisp_make_error("make-temp-file: failed to create temp file");

    return lisp_make_string(buf);
}

/* Return directory part of a path (everything before last separator, including separator).
 * Emacs Lisp style: (directory-file-name PATH) and (file-name-directory PATH)
 * Returns: String with directory part, or nil if no directory.
 */
static LispObject *builtin_file_name_directory(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("file-name-directory");

    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) != LISP_STRING) {
        return lisp_make_error("file-name-directory: argument must be a string");
    }

    const char *path = LISP_STR_VAL(arg);
    const char *last_sep = strrchr(path, '/');

#if defined(_WIN32) || defined(_WIN64)
    const char *last_backslash = strrchr(path, '\\');
    if (last_backslash && (!last_sep || last_backslash > last_sep))
        last_sep = last_backslash;
#endif

    if (last_sep == NULL)
        return NIL;

    size_t len = (size_t)(last_sep - path + 1);
    char *result = GC_malloc(len + 1);
    memcpy(result, path, len);
    result[len] = '\0';

    return lisp_make_string(result);
}

/* Return non-directory part of a path (filename after last separator).
 * Emacs Lisp style: (file-name-nondirectory PATH)
 * Returns: String with filename component.
 */
static LispObject *builtin_file_name_nondirectory(LispObject *args, Environment *env)
{
    (void)env;
    CHECK_ARGS_1("file-name-nondirectory");

    LispObject *arg = lisp_car(args);
    if (LISP_TYPE(arg) != LISP_STRING) {
        return lisp_make_error("file-name-nondirectory: argument must be a string");
    }

    const char *path = LISP_STR_VAL(arg);
    size_t path_len = strlen(path);
    if (path_len == 0)
        return lisp_make_string("");

    const char *last_sep = strrchr(path, '/');

#if defined(_WIN32) || defined(_WIN64)
    const char *last_backslash = strrchr(path, '\\');
    if (last_backslash && (!last_sep || last_backslash > last_sep))
        last_sep = last_backslash;
#endif

    if (last_sep == NULL)
        return lisp_make_string(path);

    /* If path ends with a separator, the non-directory part is empty */
    if (last_sep == path + path_len - 1)
        return lisp_make_string("");

    return lisp_make_string(last_sep + 1);
}

void register_filesystem_builtins(Environment *env)
{
    REGISTER("home-directory", builtin_home_directory);
    REGISTER("expand-path", builtin_expand_path);
    REGISTER("getenv", builtin_getenv);
    REGISTER("data-directory", builtin_data_directory);
    REGISTER("config-directory", builtin_config_directory);
    REGISTER("file-exists?", builtin_file_exists_question);
    REGISTER("mkdir", builtin_mkdir);
    REGISTER("delete-directory", builtin_delete_directory);
    REGISTER("file-is-directory?", builtin_file_is_directory_question);
    REGISTER("system-type", builtin_system_type);
    REGISTER("temporary-file-directory", builtin_temporary_file_directory);
    REGISTER("make-temp-file", builtin_make_temp_file);
    REGISTER("current-directory", builtin_current_directory);
    REGISTER("pwd", builtin_current_directory);
    REGISTER("file-name-directory", builtin_file_name_directory);
    REGISTER("directory-file-name", builtin_file_name_directory);
    REGISTER("file-name-nondirectory", builtin_file_name_nondirectory);
}
