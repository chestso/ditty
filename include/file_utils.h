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

/* Resolve a library name by searching load paths for name.lisp or name/name.lisp.
 * Search order (first match wins):
 *   1. Current working directory
 *   2. Each dir in DITTY_LISP_PATH (colon-separated on Unix, semicolon on Windows)
 *   3. $XDG_DATA_HOME/ditty/lisp/ (default: ~/.local/share/ditty/lisp/)
 *   4. Each dir in $XDG_DATA_DIRS/ditty/lisp/ (default: /usr/local/share:/usr/share)
 * On Windows:
 *   3. %APPDATA%\ditty\lisp\
 *   4. Exe-relative ..\share\ditty\lisp\
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
