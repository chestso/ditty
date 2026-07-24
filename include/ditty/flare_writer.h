/* flare_writer.h - Flare output writer interface
 *
 * Writers receive styled output bytes from formatters. They are
 * the terminal point of the Flare v3.0 pipeline.
 */

#ifndef DITTY_FLARE_WRITER_H
#define DITTY_FLARE_WRITER_H

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

typedef struct FlareWriter FlareWriter;

typedef struct
{
    /* Write data. Returns bytes written, negative on error. */
    ssize_t (*write)(FlareWriter *w, const void *data, size_t len);

    /* Flush buffered data. Returns 0 on success, negative on error. */
    int (*flush)(FlareWriter *w);

    /* Destroy writer. */
    void (*free)(FlareWriter *w);
} FlareWriterVTable;

struct FlareWriter
{
    const FlareWriterVTable *vtable;
};

/* Write helper */
ssize_t flare_writer_write(FlareWriter *w, const void *data, size_t len);

/* Flush helper */
int flare_writer_flush(FlareWriter *w);

/* Destroy writer */
void flare_writer_free(FlareWriter *w);

/* Built-in writers */
FlareWriter *flare_writer_file(FILE *f);
FlareWriter *flare_writer_fd(int fd);
FlareWriter *flare_writer_buffer(void);
FlareWriter *flare_writer_null(void);

/* Buffer writer accessors */
const char *flare_writer_buffer_data(FlareWriter *w);
size_t flare_writer_buffer_len(FlareWriter *w);
char *flare_writer_buffer_steal(FlareWriter *w, size_t *out_len);

#endif /* DITTY_FLARE_WRITER_H */
