/* flare_source.h - Flare input source interface
 *
 * Sources provide raw bytes to lexers on demand. They are the
 * starting point of the Flare v3.0 pull-based streaming pipeline.
 */

#ifndef DITTY_FLARE_SOURCE_H
#define DITTY_FLARE_SOURCE_H

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct FlareSource FlareSource;

typedef struct
{
    /* Read up to `len` bytes into `buf`.
     * Returns bytes read, 0 on EOF, negative on error. */
    ssize_t (*read)(FlareSource *src, char *buf, size_t len);

    /* Seek to absolute byte offset (optional — may be NULL).
     * Returns 0 on success, negative on error or if not seekable. */
    int (*seek)(FlareSource *src, size_t offset);

    /* Get source name for error messages (e.g., filename). May return NULL. */
    const char *(*name)(FlareSource *src);

    /* Destroy source and release all resources. */
    void (*free)(FlareSource *src);
} FlareSourceVTable;

struct FlareSource
{
    const FlareSourceVTable *vtable;
};

/* Read helper */
ssize_t flare_source_read(FlareSource *src, char *buf, size_t len);

/* Seek helper. Returns 0 on success, negative on error or if not seekable. */
int flare_source_seek(FlareSource *src, size_t offset);

/* Get source name. May return NULL. */
const char *flare_source_name(FlareSource *src);

/* Destroy source */
void flare_source_free(FlareSource *src);

/* Built-in sources */
FlareSource *flare_source_string(const char *str, size_t len, int owned);
FlareSource *flare_source_file(FILE *f, const char *name);
FlareSource *flare_source_fd(int fd, const char *name);
FlareSource *flare_source_file_contents(const char *path);

#endif /* DITTY_FLARE_SOURCE_H */
