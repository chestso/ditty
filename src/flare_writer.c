/* flare_writer.c - Flare output writer implementations
 *
 * Writers receive styled output bytes from formatters. They are the
 * terminal point of the Flare v3.0 pipeline.
 */

#include "../include/ditty/flare_writer.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ----- Buffer writer ----- */

typedef struct
{
    FlareWriter base;
    char *data;
    size_t len;
    size_t cap;
} BufferWriter;

static ssize_t buffer_write(FlareWriter *w, const void *data, size_t len)
{
    BufferWriter *bw = (BufferWriter *)w;
    if (bw->len + len > bw->cap) {
        size_t new_cap = bw->cap ? bw->cap * 2 : 256;
        while (new_cap < bw->len + len)
            new_cap *= 2;
        char *new_data = realloc(bw->data, new_cap);
        if (!new_data)
            return -1;
        bw->data = new_data;
        bw->cap = new_cap;
    }
    memcpy(bw->data + bw->len, data, len);
    bw->len += len;
    return (ssize_t)len;
}

static int buffer_flush(FlareWriter *w)
{
    (void)w;
    return 0;
}

static void buffer_free(FlareWriter *w)
{
    BufferWriter *bw = (BufferWriter *)w;
    free(bw->data);
    free(bw);
}

static const FlareWriterVTable buffer_vtable = {
    .write = buffer_write,
    .flush = buffer_flush,
    .free = buffer_free
};

FlareWriter *flare_writer_buffer(void)
{
    BufferWriter *bw = calloc(1, sizeof(BufferWriter));
    if (!bw)
        return NULL;
    bw->base.vtable = &buffer_vtable;
    return &bw->base;
}

const char *flare_writer_buffer_data(FlareWriter *w)
{
    BufferWriter *bw = (BufferWriter *)w;
    return bw ? bw->data : NULL;
}

size_t flare_writer_buffer_len(FlareWriter *w)
{
    BufferWriter *bw = (BufferWriter *)w;
    return bw ? bw->len : 0;
}

char *flare_writer_buffer_steal(FlareWriter *w, size_t *out_len)
{
    BufferWriter *bw = (BufferWriter *)w;
    if (!bw)
        return NULL;
    char *data = bw->data;
    if (out_len)
        *out_len = bw->len;
    bw->data = NULL;
    bw->len = 0;
    bw->cap = 0;
    return data;
}

/* ----- File writer ----- */

typedef struct
{
    FlareWriter base;
    FILE *f;
    int close_on_free;
} FileWriter;

static ssize_t file_write(FlareWriter *w, const void *data, size_t len)
{
    FileWriter *fw = (FileWriter *)w;
    size_t n = fwrite(data, 1, len, fw->f);
    if (n < len && ferror(fw->f))
        return -1;
    return (ssize_t)n;
}

static int file_flush(FlareWriter *w)
{
    FileWriter *fw = (FileWriter *)w;
    return fflush(fw->f) == 0 ? 0 : -1;
}

static void file_free(FlareWriter *w)
{
    FileWriter *fw = (FileWriter *)w;
    if (fw->close_on_free && fw->f)
        fclose(fw->f);
    free(fw);
}

static const FlareWriterVTable file_vtable = {
    .write = file_write,
    .flush = file_flush,
    .free = file_free
};

FlareWriter *flare_writer_file(FILE *f)
{
    if (!f)
        return NULL;
    FileWriter *fw = calloc(1, sizeof(FileWriter));
    if (!fw)
        return NULL;
    fw->base.vtable = &file_vtable;
    fw->f = f;
    fw->close_on_free = 0;
    return &fw->base;
}

/* ----- FD writer ----- */

typedef struct
{
    FlareWriter base;
    int fd;
    int close_on_free;
} FdWriter;

static ssize_t fd_write(FlareWriter *w, const void *data, size_t len)
{
    FdWriter *fdw = (FdWriter *)w;
    ssize_t n = write(fdw->fd, data, len);
    if (n < 0)
        return -1;
    return n;
}

static int fd_flush(FlareWriter *w)
{
    (void)w;
    return 0;
}

static void fd_free(FlareWriter *w)
{
    FdWriter *fdw = (FdWriter *)w;
    if (fdw->close_on_free && fdw->fd >= 0)
        close(fdw->fd);
    free(fdw);
}

static const FlareWriterVTable fd_vtable = {
    .write = fd_write,
    .flush = fd_flush,
    .free = fd_free
};

FlareWriter *flare_writer_fd(int fd)
{
    FdWriter *fdw = calloc(1, sizeof(FdWriter));
    if (!fdw)
        return NULL;
    fdw->base.vtable = &fd_vtable;
    fdw->fd = fd;
    fdw->close_on_free = 0;
    return &fdw->base;
}

/* ----- Null writer ----- */

typedef struct
{
    FlareWriter base;
} NullWriter;

static ssize_t null_write(FlareWriter *w, const void *data, size_t len)
{
    (void)w;
    (void)data;
    return (ssize_t)len;
}

static int null_flush(FlareWriter *w)
{
    (void)w;
    return 0;
}

static void null_free(FlareWriter *w)
{
    free(w);
}

static const FlareWriterVTable null_vtable = {
    .write = null_write,
    .flush = null_flush,
    .free = null_free
};

FlareWriter *flare_writer_null(void)
{
    NullWriter *nw = calloc(1, sizeof(NullWriter));
    if (!nw)
        return NULL;
    nw->base.vtable = &null_vtable;
    return &nw->base;
}

/* ----- VTable helpers ----- */

ssize_t flare_writer_write(FlareWriter *w, const void *data, size_t len)
{
    if (!w || !w->vtable || !w->vtable->write)
        return -1;
    return w->vtable->write(w, data, len);
}

int flare_writer_flush(FlareWriter *w)
{
    if (!w || !w->vtable || !w->vtable->flush)
        return 0;
    return w->vtable->flush(w);
}

void flare_writer_free(FlareWriter *w)
{
    if (!w)
        return;
    if (w->vtable && w->vtable->free)
        w->vtable->free(w);
}
