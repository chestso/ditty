/* Cross-platform file utilities with UTF-8 support */

#include "../include/file_utils.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <wchar.h>
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

#ifdef _WIN32
/* Convert UTF-8 string to UTF-16 (wide char) for Windows APIs.
 * Caller must free the returned buffer with free().
 *
 * Returns: Wide char string on success, NULL on failure
 */
wchar_t *utf8_to_utf16(const char *utf8_str)
{
    if (!utf8_str)
        return NULL;

    /* Get required buffer size */
    int wchar_count = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    if (wchar_count == 0)
        return NULL;

    /* Allocate buffer */
    wchar_t *wchar_str = (wchar_t *)malloc(wchar_count * sizeof(wchar_t));
    if (!wchar_str)
        return NULL;

    /* Convert UTF-8 to UTF-16 */
    if (MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wchar_str, wchar_count) == 0) {
        free(wchar_str);
        return NULL;
    }

    return wchar_str;
}
#endif

FILE *file_open(const char *utf8_path, const char *mode)
{
    if (!utf8_path || !mode)
        return NULL;

#ifdef _WIN32
    /* Force binary mode on Windows. The Lisp-level EOL handling
     * (write_with_eol_translation, detect_eol_from_file, read-line)
     * does all \n↔\r\n translation explicitly. C runtime text mode
     * would double-translate on write and strip \r\n on read. */
    char binmode[8];
    const char *eff_mode = mode;
    if (!strchr(mode, 'b')) {
        snprintf(binmode, sizeof(binmode), "%sb", mode);
        eff_mode = binmode;
    }

    /* Convert UTF-8 path and mode to UTF-16 */
    wchar_t *wpath = utf8_to_utf16(utf8_path);
    if (!wpath)
        return NULL;

    wchar_t *wmode = utf8_to_utf16(eff_mode);
    if (!wmode) {
        free(wpath);
        return NULL;
    }

    /* Open file with wide char API */
    FILE *file = _wfopen(wpath, wmode);

    /* Clean up */
    free(wpath);
    free(wmode);

    return file;
#else
    /* On Unix/macOS, fopen() already handles UTF-8 */
    return fopen(utf8_path, mode);
#endif
}

int file_remove(const char *utf8_path)
{
    if (!utf8_path)
        return -1;

#ifdef _WIN32
    /* Convert UTF-8 path to UTF-16 */
    wchar_t *wpath = utf8_to_utf16(utf8_path);
    if (!wpath)
        return -1;

    /* Remove file with wide char API */
    int result = _wremove(wpath);

    /* Clean up */
    free(wpath);

    return result;
#else
    /* On Unix/macOS, remove() already handles UTF-8 */
    return remove(utf8_path);
#endif
}

int file_exists(const char *path)
{
    if (!path)
        return 0;

#ifdef _WIN32
    wchar_t *wpath = utf8_to_utf16(path);
    if (!wpath)
        return 0;

    struct _stat st;
    int result = _wstat(wpath, &st) == 0;
    free(wpath);
    return result;
#else
    struct stat st;
    return stat(path, &st) == 0;
#endif
}

int file_is_directory(const char *path)
{
    if (!path)
        return 0;

#ifdef _WIN32
    wchar_t *wpath = utf8_to_utf16(path);
    if (!wpath)
        return 0;

    struct _stat st;
    int result = 0;
    if (_wstat(wpath, &st) == 0)
        result = (st.st_mode & _S_IFDIR) != 0;
    free(wpath);
    return result;
#else
    struct stat st;
    if (stat(path, &st) != 0)
        return 0;
    return S_ISDIR(st.st_mode);
#endif
}

#ifdef _WIN32
static int remove_directory_recursive_win32(const wchar_t *wpath_in)
{
    size_t len = wcslen(wpath_in);
    wchar_t *wpath = malloc((len + 1) * sizeof(wchar_t));
    if (!wpath)
        return -1;
    wcscpy(wpath, wpath_in);
    for (wchar_t *p = wpath; *p; p++) {
        if (*p == L'/')
            *p = L'\\';
    }
    len = wcslen(wpath);
    while (len > 0 && wpath[len - 1] == L'\\') {
        wpath[len - 1] = L'\0';
        len--;
    }

    wchar_t *pattern = malloc((len + 4) * sizeof(wchar_t));
    if (!pattern) {
        free(wpath);
        return -1;
    }
    _snwprintf(pattern, len + 4, L"%s\\*", wpath);

    WIN32_FIND_DATAW fd;
    HANDLE hfind = FindFirstFileW(pattern, &fd);
    free(pattern);

    if (hfind == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        free(wpath);
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
            return RemoveDirectoryW(wpath) ? 0 : -1;
        errno = EACCES;
        return -1;
    }

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0)
            continue;

        size_t entry_len = len + 1 + wcslen(fd.cFileName) + 1;
        wchar_t *entry = malloc(entry_len * sizeof(wchar_t));
        if (!entry) {
            FindClose(hfind);
            free(wpath);
            return -1;
        }
        _snwprintf(entry, entry_len, L"%s\\%s", wpath, fd.cFileName);

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            remove_directory_recursive_win32(entry);
        else
            DeleteFileW(entry);

        free(entry);
    } while (FindNextFileW(hfind, &fd));

    FindClose(hfind);

    int result = RemoveDirectoryW(wpath);
    free(wpath);
    if (!result)
        errno = EACCES;
    return result ? 0 : -1;
}
#endif

int file_remove_directory(const char *utf8_path, int recursive)
{
    if (!utf8_path)
        return -1;

#ifdef _WIN32
    /* Normalize path to backslashes for reliable Win32 API behavior */
    size_t pathlen = strlen(utf8_path);
    char *normalized = malloc(pathlen + 1);
    if (!normalized)
        return -1;
    memcpy(normalized, utf8_path, pathlen + 1);
    for (size_t i = 0; i < pathlen; i++) {
        if (normalized[i] == '/')
            normalized[i] = '\\';
    }

    if (!file_is_directory(normalized)) {
        fprintf(stderr, "DEBUG: file_is_directory failed for: %s\n", normalized);
        free(normalized);
        errno = ENOTDIR;
        return -1;
    }

    wchar_t *wpath = utf8_to_utf16(normalized);
    free(normalized);
    if (!wpath)
        return -1;

    int result;
    if (recursive) {
        result = remove_directory_recursive_win32(wpath);
    } else {
        result = RemoveDirectoryW(wpath) ? 0 : -1;
    }
    free(wpath);
    if (result != 0)
        errno = EACCES;
    return result;
#else
    if (!file_is_directory(utf8_path))
        return -1;

    if (recursive) {
        size_t pathlen = strlen(utf8_path);
        char *pattern = malloc(pathlen + 3);
        if (!pattern)
            return -1;
        snprintf(pattern, pathlen + 3, "%s/.", utf8_path);

        DIR *d = opendir(pattern);
        if (!d) {
            free(pattern);
            return rmdir(utf8_path);
        }

        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;

            size_t entry_len = pathlen + 1 + strlen(ent->d_name) + 1;
            char *entry = malloc(entry_len);
            if (!entry)
                continue;
            snprintf(entry, entry_len, "%s/%s", utf8_path, ent->d_name);

            if (file_is_directory(entry))
                file_remove_directory(entry, 1);
            else
                unlink(entry);

            free(entry);
        }
        closedir(d);
        free(pattern);
        return rmdir(utf8_path);
    } else {
        return rmdir(utf8_path);
    }
#endif
}

/* Search cwd and XDG data dirs for a single filename */
static const char *file_resolve_one(const char *filename, char *resolved, size_t resolved_size)
{
    /* Try relative to cwd first */
    if (file_exists(filename))
        return filename;

    /* Try DITTY_LISP_PATH directories */
    const char *lisp_path = getenv("DITTY_LISP_PATH");
    if (lisp_path && lisp_path[0]) {
        size_t path_len = strlen(lisp_path);
        char *path_copy = malloc(path_len + 1);
        if (path_copy) {
            memcpy(path_copy, lisp_path, path_len + 1);
            char *saveptr = NULL;
#ifdef _WIN32
            const char *sep = ";";
#else
            const char *sep = ":";
#endif
            char *dir = strtok_r(path_copy, sep, &saveptr);
            while (dir) {
                snprintf(resolved, resolved_size, "%s%c%s", dir, PATH_SEP, filename);
                if (file_exists(resolved)) {
                    free(path_copy);
                    return resolved;
                }
                dir = strtok_r(NULL, sep, &saveptr);
            }
            free(path_copy);
        }
    }

#ifdef _WIN32
    /* Try %APPDATA%/ditty/ on Windows */
    const char *appdata = getenv("APPDATA");
    if (appdata && appdata[0]) {
        snprintf(resolved, resolved_size, "%s\\ditty\\%s", appdata, filename);
        if (file_exists(resolved))
            return resolved;
    }
    /* Try exe-relative ..\share\ditty\ */
    wchar_t wpath[MAX_PATH];
    if (GetModuleFileNameW(NULL, wpath, MAX_PATH) > 0) {
        char exe_path[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, wpath, -1, exe_path, MAX_PATH, NULL, NULL);
        char *last_sep = strrchr(exe_path, '\\');
        if (last_sep) {
            *last_sep = '\0';
            snprintf(resolved, resolved_size, "%s\\share\\ditty\\%s", exe_path, filename);
            if (file_exists(resolved))
                return resolved;
        }
    }
#else
    /* Try XDG_DATA_HOME/ditty/ */
    const char *data_home = getenv("XDG_DATA_HOME");
    if (data_home && data_home[0]) {
        snprintf(resolved, resolved_size, "%s/ditty/%s", data_home, filename);
        if (file_exists(resolved))
            return resolved;
    } else {
        const char *home = getenv("HOME");
        if (home) {
            snprintf(resolved, resolved_size, "%s/.local/share/ditty/%s", home, filename);
            if (file_exists(resolved))
                return resolved;
        }
    }

    /* Try each dir in XDG_DATA_DIRS/ditty/ */
    const char *data_dirs = getenv("XDG_DATA_DIRS");
    if (!data_dirs || !data_dirs[0])
        data_dirs = "/usr/local/share:/usr/share";

    /* Copy so we can tokenize */
    size_t dirs_len = strlen(data_dirs);
    char *dirs_copy = malloc(dirs_len + 1);
    if (!dirs_copy)
        return NULL;
    memcpy(dirs_copy, data_dirs, dirs_len + 1);

    char *saveptr = NULL;
    char *dir = strtok_r(dirs_copy, ":", &saveptr);
    while (dir) {
        snprintf(resolved, resolved_size, "%s/ditty/%s", dir, filename);
        if (file_exists(resolved)) {
            free(dirs_copy);
            return resolved;
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }
    free(dirs_copy);
#endif

    return NULL;
}

const char *file_resolve(const char *filename, char *resolved, size_t resolved_size)
{
    if (!filename || !resolved || resolved_size == 0)
        return NULL;

    /* Absolute or explicitly relative paths: use as-is */
#ifdef _WIN32
    if (filename[0] == '/' || filename[0] == '\\' ||
        (filename[0] && filename[1] == ':') ||
        (filename[0] == '.' && (filename[1] == '/' || filename[1] == '\\' || filename[1] == '.')))
        return filename;
#else
    if (filename[0] == '/' || (filename[0] == '.' && (filename[1] == '/' || filename[1] == '.')))
        return filename;
#endif

    /* Try exact name first */
    const char *result = file_resolve_one(filename, resolved, resolved_size);
    if (result)
        return result;

    size_t len = strlen(filename);
    int has_lisp_ext = (len >= 5 && strcmp(filename + len - 5, ".lisp") == 0);
    int has_lisp_prefix = (len >= 5 && strncmp(filename, "lisp/", 5) == 0);

    /* Try with .lisp extension */
    if (!has_lisp_ext) {
        char name_buf[4096];
        snprintf(name_buf, sizeof(name_buf), "%s.lisp", filename);
        result = file_resolve_one(name_buf, resolved, resolved_size);
        if (result)
            return result;
    }

    /* Try with lisp/ prefix */
    if (!has_lisp_prefix) {
        char name_buf[4096];
        snprintf(name_buf, sizeof(name_buf), "lisp/%s", filename);
        result = file_resolve_one(name_buf, resolved, resolved_size);
        if (result)
            return result;
    }

    /* Try with both lisp/ prefix and .lisp extension */
    if (!has_lisp_prefix && !has_lisp_ext) {
        char name_buf[4096];
        snprintf(name_buf, sizeof(name_buf), "lisp/%s.lisp", filename);
        result = file_resolve_one(name_buf, resolved, resolved_size);
        if (result)
            return result;
    }

    /* Not found anywhere, return original (will fail at open) */
    return filename;
}

/* Try to find a library file in a single directory.
 * Tries <dir>/<name>.lisp, then <dir>/<name>/<name>.lisp.
 * Returns resolved path on success, NULL on failure.
 */
static const char *try_library_in_dir(const char *dir, const char *name,
                                      char *resolved, size_t resolved_size)
{
    /* Try <dir>/<name>.lisp */
    snprintf(resolved, resolved_size, "%s%c%s.lisp", dir, PATH_SEP, name);
    if (file_exists(resolved))
        return resolved;

    /* Try <dir>/<name>/<name>.lisp */
    snprintf(resolved, resolved_size, "%s%c%s%c%s.lisp", dir, PATH_SEP, name, PATH_SEP, name);
    if (file_exists(resolved))
        return resolved;

    return NULL;
}

const char *file_resolve_library(const char *name, char *resolved, size_t resolved_size)
{
    if (!name || !name[0] || !resolved || resolved_size == 0)
        return NULL;

    char path_buf[4096];

    /* 1. Try current working directory */
    if (try_library_in_dir(".", name, resolved, resolved_size))
        return resolved;

    /* 2. Try DITTY_LISP_PATH directories */
    const char *lisp_path = getenv("DITTY_LISP_PATH");
    if (lisp_path && lisp_path[0]) {
        size_t path_len = strlen(lisp_path);
        char *path_copy = malloc(path_len + 1);
        if (path_copy) {
            memcpy(path_copy, lisp_path, path_len + 1);
            char *saveptr = NULL;
#ifdef _WIN32
            const char *sep = ";";
#else
            const char *sep = ":";
#endif
            char *dir = strtok_r(path_copy, sep, &saveptr);
            while (dir) {
                if (try_library_in_dir(dir, name, resolved, resolved_size)) {
                    free(path_copy);
                    return resolved;
                }
                dir = strtok_r(NULL, sep, &saveptr);
            }
            free(path_copy);
        }
    }

#ifdef _WIN32
    /* 3. Try %APPDATA%\ditty\lisp\ */
    const char *appdata = getenv("APPDATA");
    if (appdata && appdata[0]) {
        snprintf(path_buf, sizeof(path_buf), "%s\\ditty\\lisp", appdata);
        if (try_library_in_dir(path_buf, name, resolved, resolved_size))
            return resolved;
    }

    /* 4. Try exe-relative ..\share\ditty\lisp\ */
    wchar_t wpath[MAX_PATH];
    if (GetModuleFileNameW(NULL, wpath, MAX_PATH) > 0) {
        char exe_path[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, wpath, -1, exe_path, MAX_PATH, NULL, NULL);
        /* Strip filename to get exe directory */
        char *last_sep = strrchr(exe_path, '\\');
        if (last_sep) {
            *last_sep = '\0';
            snprintf(path_buf, sizeof(path_buf), "%s\\share\\ditty\\lisp", exe_path);
            if (try_library_in_dir(path_buf, name, resolved, resolved_size))
                return resolved;
        }
    }
#else
    /* 3. Try XDG_DATA_HOME/ditty/lisp/ */
    const char *data_home = getenv("XDG_DATA_HOME");
    if (data_home && data_home[0]) {
        snprintf(path_buf, sizeof(path_buf), "%s/ditty/lisp", data_home);
        if (try_library_in_dir(path_buf, name, resolved, resolved_size))
            return resolved;
    } else {
        const char *home = getenv("HOME");
        if (home) {
            snprintf(path_buf, sizeof(path_buf), "%s/.local/share/ditty/lisp", home);
            if (try_library_in_dir(path_buf, name, resolved, resolved_size))
                return resolved;
        }
    }

    /* 4. Try each dir in XDG_DATA_DIRS/ditty/lisp/ */
    const char *data_dirs = getenv("XDG_DATA_DIRS");
    if (!data_dirs || !data_dirs[0])
        data_dirs = "/usr/local/share:/usr/share";

    size_t dirs_len = strlen(data_dirs);
    char *dirs_copy = malloc(dirs_len + 1);
    if (dirs_copy) {
        memcpy(dirs_copy, data_dirs, dirs_len + 1);
        char *saveptr = NULL;
        char *dir = strtok_r(dirs_copy, ":", &saveptr);
        while (dir) {
            snprintf(path_buf, sizeof(path_buf), "%s/ditty/lisp", dir);
            if (try_library_in_dir(path_buf, name, resolved, resolved_size)) {
                free(dirs_copy);
                return resolved;
            }
            dir = strtok_r(NULL, ":", &saveptr);
        }
        free(dirs_copy);
    }
#endif

    return NULL;
}

int file_mkdir(const char *utf8_path)
{
    if (!utf8_path)
        return -1;

#ifdef _WIN32
    /* Convert UTF-8 path to UTF-16 */
    wchar_t *wpath = utf8_to_utf16(utf8_path);
    if (!wpath)
        return -1;

    /* Create directory with wide char API */
    int result = _wmkdir(wpath);

    /* Clean up */
    free(wpath);

    return result;
#else
    /* On Unix/macOS, mkdir() already handles UTF-8 */
    return mkdir(utf8_path, 0755);
#endif
}

int file_mkdir_p(const char *path)
{
    if (!path || !path[0])
        return -1;

    size_t len = strlen(path);
    char *buf = malloc(len + 1);
    if (!buf)
        return -1;
    memcpy(buf, path, len + 1);

    /* Walk path components and create each missing directory */
    for (size_t i = 1; i <= len; i++) {
        if (buf[i] == '/' || buf[i] == '\\' || buf[i] == '\0') {
            char saved = buf[i];
            buf[i] = '\0';

            if (!file_exists(buf)) {
                if (file_mkdir(buf) != 0 && errno != EEXIST) {
                    free(buf);
                    return -1;
                }
            }

            buf[i] = saved;
        }
    }

    free(buf);
    return 0;
}

int file_temp_directory(char *buf, size_t size)
{
    if (!buf || size == 0)
        return -1;

#ifdef _WIN32
    DWORD len = GetTempPathA((DWORD)size, buf);
    if (len == 0 || len >= size)
        return -1;
    /* GetTempPathA returns a trailing backslash; strip it */
    if (len > 0 && (buf[len - 1] == '\\' || buf[len - 1] == '/'))
        buf[len - 1] = '\0';
    return 0;
#else
    const char *tmp = getenv("TMPDIR");
    if (!tmp || !tmp[0])
        tmp = "/tmp";
    if (strlen(tmp) >= size)
        return -1;
    strcpy(buf, tmp);
    return 0;
#endif
}

int file_make_temp_file(const char *prefix, char *buf, size_t buf_size)
{
    if (!prefix || !buf || buf_size == 0)
        return -1;

    char tmpdir[4096];
    if (file_temp_directory(tmpdir, sizeof(tmpdir)) != 0)
        return -1;

#ifdef _WIN32
    /* Build a template: <tmpdir>\<prefix>XXXXXX and use GetTempFileName */
    char template_buf[8192];
    int n = snprintf(template_buf, sizeof(template_buf), "%s\\%s", tmpdir,
                     prefix);
    if (n < 0 || (size_t)n >= sizeof(template_buf))
        return -1;

    /* GetTempFileNameA needs a unique ID; it creates the file */
    UINT uniqueId = 0;
    char tmpPath[MAX_PATH];
    /* GetTempPathA for the directory part */
    char dirPart[MAX_PATH];
    DWORD dlen = GetTempPathA(MAX_PATH, dirPart);
    if (dlen == 0 || dlen >= MAX_PATH)
        return -1;

    if (!GetTempFileNameA(dirPart, prefix, uniqueId, tmpPath))
        return -1;

    if (strlen(tmpPath) >= buf_size)
        return -1;
    strcpy(buf, tmpPath);
    return 0;
#else
    /* Build template: <tmpdir>/<prefix>XXXXXX */
    int n = snprintf(buf, buf_size, "%s/%sXXXXXX", tmpdir, prefix);
    if (n < 0 || (size_t)n >= buf_size)
        return -1;

    int fd = mkstemp(buf);
    if (fd < 0)
        return -1;
    close(fd);
    return 0;
#endif
}

int file_make_temp_dir(const char *prefix, char *buf, size_t buf_size)
{
    if (!prefix || !buf || buf_size == 0)
        return -1;

    char tmpdir[4096];
    if (file_temp_directory(tmpdir, sizeof(tmpdir)) != 0)
        return -1;

#ifdef _WIN32
    /* On Windows, create a unique directory by retrying with random suffixes */
    for (int attempt = 0; attempt < 100; attempt++) {
        char name[32];
        snprintf(name, sizeof(name), "%s%06d", prefix,
                 (int)(rand() % 1000000));
        int n = snprintf(buf, buf_size, "%s\\%s", tmpdir, name);
        if (n < 0 || (size_t)n >= buf_size)
            return -1;
        if (CreateDirectoryA(buf, NULL))
            return 0;
        if (GetLastError() != ERROR_ALREADY_EXISTS)
            return -1;
    }
    return -1;
#else
    int n = snprintf(buf, buf_size, "%s/%sXXXXXX", tmpdir, prefix);
    if (n < 0 || (size_t)n >= buf_size)
        return -1;

    char *result = mkdtemp(buf);
    if (result == NULL)
        return -1;
    return 0;
#endif
}
