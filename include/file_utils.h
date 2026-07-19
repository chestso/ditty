/* Cross-platform file utilities with UTF-8 support */

#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdio.h>

/* Cross-platform file open with UTF-8 path support.
 * On Windows: converts UTF-8 path to UTF-16 and uses _wfopen()
 * On Unix/macOS: calls fopen() directly
 *
 * Parameters:
 *   utf8_path - File path in UTF-8 encoding
 *   mode - Standard fopen() mode string ("r", "w", "rb", etc.)
 *
 * Returns:
 *   FILE* on success, NULL on failure
 */
FILE *file_open(const char *utf8_path, const char *mode);

/* Cross-platform file removal with UTF-8 path support.
 * On Windows: converts UTF-8 path to UTF-16 and uses _wremove()
 * On Unix/macOS: calls remove() directly
 *
 * Parameters:
 *   utf8_path - File path in UTF-8 encoding
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int file_remove(const char *utf8_path);

/* Cross-platform directory creation with UTF-8 path support.
 * On Windows: converts UTF-8 path to UTF-16 and uses _wmkdir()
 * On Unix/macOS: calls mkdir() directly with 0755 permissions
 *
 * Parameters:
 *   utf8_path - Directory path in UTF-8 encoding
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int file_mkdir(const char *utf8_path);

/* Check if a file or directory exists.
 * On Windows: converts UTF-8 path to UTF-16 and uses _wstat()
 * On Unix/macOS: calls stat() directly
 *
 * Parameters:
 *   path - File path in UTF-8 encoding
 *
 * Returns:
 *   1 if exists, 0 if not
 */
int file_exists(const char *path);

/* Check if a path is a directory.
 * Returns 1 if path exists and is a directory, 0 otherwise.
 */
int file_is_directory(const char *path);

/* Remove a directory.
 * If recursive is 0, only removes empty directories (like rmdir).
 * If recursive is 1, removes the directory and all its contents.
 *
 * Returns 0 on success, -1 on failure.
 */
int file_remove_directory(const char *utf8_path, int recursive);

/* Recursive directory creation (like mkdir -p).
 * Creates all missing parent directories along the path.
 * Succeeds silently if directory already exists.
 *
 * Parameters:
 *   path - Directory path in UTF-8 encoding
 *
 * Returns:
 *   0 on success, -1 on failure
 */
int file_mkdir_p(const char *path);

/* Resolve a file path by searching load paths and XDG data directories.
 * For relative paths (not starting with / ./ ../), searches:
 *   1. Current working directory
 *   2. Directories in DITTY_LISP_PATH (colon-separated on Unix, semicolon on Windows)
 *   3. $XDG_DATA_HOME/ditty/ (default: ~/.local/share/ditty/)
 *   4. Each dir in $XDG_DATA_DIRS/ditty/ (default: /usr/local/share:/usr/share)
 * On Windows: %APPDATA%\ditty\ and exe-relative ..\share\ditty\
 *
 * Parameters:
 *   filename - File path to resolve
 *   resolved - Output buffer for resolved path
 *   resolved_size - Size of output buffer
 *
 * Returns:
 *   resolved on success (points to the found path), NULL if not found anywhere
 */
const char *file_resolve(const char *filename, char *resolved, size_t resolved_size);

/* Set the directory of the main script being executed.
 * Pass NULL or "" to clear. Used by the CLI so that `require` can find
 * libraries sitting next to the entry script, independent of CWD.
 *
 * Parameters:
 *   dir - Absolute directory path (UTF-8), or NULL to clear. The value is
 *         copied into an internal buffer; the caller may free/reuse `dir`.
 */
void file_set_main_script_dir(const char *dir);

/* Get the currently-set main script directory.
 * Returns an empty string if none has been set. The returned pointer is
 * valid until the next call to file_set_main_script_dir().
 */
const char *file_get_main_script_dir(void);

/* Resolve a path to an absolute, normalized form.
 * On POSIX: uses realpath() (resolves symlinks, "." and ".." components).
 * On Windows: uses GetFullPathNameW (resolves "." and "..", does not
 *             resolve symlinks).
 * The input path must exist for realpath() to succeed on POSIX; on Windows
 * the path need not exist.
 *
 * Parameters:
 *   path - File path in UTF-8 encoding (relative or absolute)
 *   buf  - Output buffer for the absolute path
 *   size - Size of output buffer
 *
 * Returns:
 *   buf on success (NUL-terminated absolute path), NULL on failure
 *   (path does not exist, buffer too small, or conversion error)
 */
const char *file_absolute_path(const char *path, char *buf, size_t size);

/* Compute the directory portion of a path.
 * Strips the final path component (filename) and returns the directory.
 * Handles both '/' and '\\' separators (Windows). Examples:
 *   "/a/b/c.lisp"  -> "/a/b"
 *   "/a/b/"        -> "/a"      (trailing separator dropped)
 *   "c.lisp"       -> "."
 *   "."            -> "."
 *   "/"            -> "/"
 *   "C:\\x\\y.lisp" -> "C:\\x"
 * Always returns a non-NULL, non-empty string. On buffer-too-small or
 * NULL inputs, writes "." (or nothing) and returns "buf".
 *
 * Parameters:
 *   path - File path in UTF-8 encoding (may be relative or absolute)
 *   buf  - Output buffer for the directory portion
 *   size - Size of output buffer
 *
 * Returns:
 *   buf on success (always non-NULL; "." on degenerate input)
 */
const char *file_dirname(const char *path, char *buf, size_t size);

/* Resolve a library name by searching load paths for name.lisp or name/name.lisp.
 * Search order (first match wins):
 *   1. Main script's directory (set via file_set_main_script_dir, if non-empty)
 *   2. Current working directory
 *   3. Each dir in DITTY_LISP_PATH (colon-separated on Unix, semicolon on Windows)
 *   4. $XDG_DATA_HOME/ditty/lisp/ (default: ~/.local/share/ditty/lisp/)
 *   5. Each dir in $XDG_DATA_DIRS/ditty/lisp/ (default: /usr/local/share:/usr/share)
 * On Windows:
 *   4. %APPDATA%\ditty\lisp\
 *   5. Exe-relative ..\share\ditty\lisp\
 *
 * For each search directory, tries:
 *   a. <dir>/<name>.lisp        (single-file library)
 *   b. <dir>/<name>/<name>.lisp (directory-based library)
 *
 * Parameters:
 *   name - Library name (without path or extension)
 *   resolved - Output buffer for resolved path
 *   resolved_size - Size of output buffer
 *
 * Returns:
 *   resolved on success (points to the found path), NULL if not found
 */
const char *file_resolve_library(const char *name, char *resolved, size_t resolved_size);

/* Resolve a library name by searching an explicit list of directories.
 * Walks `dirs` in order and tries, for each directory:
 *   a. <dir>/<name>.lisp        (single-file library)
 *   b. <dir>/<name>/<name>.lisp (directory-based library)
 * Returns the first match. This is the pure core of file_resolve_library
 * without any env-var / XDG consultation; callers that already have a
 * directory list (e.g. the Lisp `*load-path*` value) can use it directly.
 *
 * Parameters:
 *   name - Library name (without path or extension)
 *   dirs - Array of directory path strings (UTF-8)
 *   n_dirs - Number of entries in `dirs`
 *   resolved - Output buffer for resolved path
 *   resolved_size - Size of output buffer
 *
 * Returns:
 *   resolved on success (points to the found path), NULL if not found
 */
const char *file_resolve_library_in_dirs(const char *name,
                                         const char *const *dirs, int n_dirs,
                                         char *resolved, size_t resolved_size);

/* Get the system temporary file directory (cross-platform).
 * On Unix: $TMPDIR or /tmp
 * On Windows: GetTempPath() (which checks %TEMP%, %TMP%, %USERPROFILE%)
 *
 * Parameters:
 *   buf - Output buffer for the directory path
 *   size - Size of output buffer
 *
 * Returns:
 *   0 on success, -1 on failure (buffer too small or cannot determine)
 */
int file_temp_directory(char *buf, size_t size);

/* Create a unique temporary file with the given prefix.
 * The file is created with 0600 permissions on Unix.
 * On Unix: uses mkstemp() with template "<dir>/<prefix>XXXXXX"
 * On Windows: uses GetTempFileNameA()
 *
 * Parameters:
 *   prefix - Prefix for the filename (may be empty string, must not be NULL)
 *   buf - Output buffer for the created file path
 *   buf_size - Size of output buffer
 *
 * Returns:
 *   0 on success (file created, path in buf), -1 on failure
 */
int file_make_temp_file(const char *prefix, char *buf, size_t buf_size);

/* Create a unique temporary directory with the given prefix.
 * The directory is created with 0700 permissions on Unix.
 * On Unix: uses mkdtemp() with template "<dir>/<prefix>XXXXXX"
 * On Windows: creates a directory with a unique name via CreateDirectory
 *
 * Parameters:
 *   prefix - Prefix for the directory name (may be empty string, must not be NULL)
 *   buf - Output buffer for the created directory path
 *   buf_size - Size of output buffer
 *
 * Returns:
 *   0 on success (directory created, path in buf), -1 on failure
 */
int file_make_temp_dir(const char *prefix, char *buf, size_t buf_size);

#ifdef _WIN32
#include <wchar.h>

/* Convert UTF-8 string to UTF-16 (wide char) for Windows APIs.
 * Caller must free the returned buffer with free().
 *
 * Parameters:
 *   utf8_str - String in UTF-8 encoding
 *
 * Returns:
 *   Wide char string on success, NULL on failure
 */
wchar_t *utf8_to_utf16(const char *utf8_str);
#endif

#endif /* FILE_UTILS_H */
