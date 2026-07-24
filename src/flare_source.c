/* flare_source.c - Flare input source implementations
 *
 * Sources provide raw bytes to lexers on demand. They are the starting
 * point of the Flare v3.0 pull-based streaming pipeline.
 */

#include "../include/ditty/flare_source.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ----- String source ----- */

typedef struct
{
    FlareSource base;
    const char *str;
    size_t len;
    size_t pos;
    int owned;
} StringSource;

static ssize_t string_read(FlareSource *src, char *buf, size_t len)
{
    StringSource *s = (StringSource *)src;
    if (s->pos >= s->len)
        return 0;
    size_t remain = s->len - s->pos;
    size_t n = len < remain ? len : remain;
    memcpy(buf, s->str + s->pos, n);
    s->pos += n;
    return (ssize_t)n;
}

static int string_seek(FlareSource *src, size_t offset)
{
    StringSource *s = (StringSource *)src;
    s->pos = offset < s->len ? offset : s->len;
    return 0;
}

static void string_free(FlareSource *src)
{
    StringSource *s = (StringSource *)src;
    if (s->owned)
        free((void *)s->str);
    free(s);
}

static const FlareSourceVTable string_vtable = {
    .read = string_read,
    .seek = string_seek,
    .name = NULL,
    .free = string_free
};

FlareSource *flare_source_string(const char *str, size_t len, int owned)
{
    StringSource *s = calloc(1, sizeof(StringSource));
    if (!s)
        return NULL;
    s->base.vtable = &string_vtable;
    s->str = str;
    s->len = len;
    s->pos = 0;
    s->owned = owned;
    return &s->base;
}

/* ----- File source ----- */

typedef struct
{
    FlareSource base;
    FILE *f;
    int close_on_free;
    char *name;
} FileSource;

static ssize_t file_read(FlareSource *src, char *buf, size_t len)
{
    FileSource *fs = (FileSource *)src;
    size_t n = fread(buf, 1, len, fs->f);
    if (n == 0 && ferror(fs->f))
        return -1;
    return (ssize_t)n;
}

static int file_seek(FlareSource *src, size_t offset)
{
    FileSource *fs = (FileSource *)src;
    return fseek(fs->f, (long)offset, SEEK_SET) == 0 ? 0 : -1;
}

static const char *file_name(FlareSource *src)
{
    FileSource *fs = (FileSource *)src;
    return fs->name;
}

static void file_free(FlareSource *src)
{
    FileSource *fs = (FileSource *)src;
    if (fs->close_on_free && fs->f)
        fclose(fs->f);
    free(fs->name);
    free(fs);
}

static const FlareSourceVTable file_vtable = {
    .read = file_read,
    .seek = file_seek,
    .name = file_name,
    .free = file_free
};

FlareSource *flare_source_file(FILE *f, const char *name)
{
    if (!f)
        return NULL;
    FileSource *fs = calloc(1, sizeof(FileSource));
    if (!fs)
        return NULL;
    fs->base.vtable = &file_vtable;
    fs->f = f;
    fs->close_on_free = 0;
    fs->name = name ? strdup(name) : NULL;
    return &fs->base;
}

/* ----- FD source ----- */

typedef struct
{
    FlareSource base;
    int fd;
    int close_on_free;
    char *name;
} FdSource;

static ssize_t fd_read(FlareSource *src, char *buf, size_t len)
{
    FdSource *fds = (FdSource *)src;
    ssize_t n = read(fds->fd, buf, len);
    if (n < 0)
        return -1;
    return n;
}

static void fd_free(FlareSource *src)
{
    FdSource *fds = (FdSource *)src;
    if (fds->close_on_free && fds->fd >= 0)
        close(fds->fd);
    free(fds->name);
    free(fds);
}

static const FlareSourceVTable fd_vtable = {
    .read = fd_read,
    .seek = NULL,
    .name = NULL,
    .free = fd_free
};

FlareSource *flare_source_fd(int fd, const char *name)
{
    FdSource *fds = calloc(1, sizeof(FdSource));
    if (!fds)
        return NULL;
    fds->base.vtable = &fd_vtable;
    fds->fd = fd;
    fds->close_on_free = 0;
    fds->name = name ? strdup(name) : NULL;
    return &fds->base;
}

FlareSource *flare_source_file_contents(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    FileSource *fs = (FileSource *)flare_source_file(f, path);
    if (!fs) {
        fclose(f);
        return NULL;
    }
    fs->close_on_free = 1;
    return &fs->base;
}

/* ----- VTable helpers ----- */

ssize_t flare_source_read(FlareSource *src, char *buf, size_t len)
{
    if (!src || !src->vtable || !src->vtable->read)
        return -1;
    return src->vtable->read(src, buf, len);
}

int flare_source_seek(FlareSource *src, size_t offset)
{
    if (!src || !src->vtable || !src->vtable->seek)
        return -1;
    return src->vtable->seek(src, offset);
}

const char *flare_source_name(FlareSource *src)
{
    if (!src || !src->vtable || !src->vtable->name)
        return NULL;
    return src->vtable->name(src);
}

void flare_source_free(FlareSource *src)
{
    if (!src)
        return;
    if (src->vtable && src->vtable->free)
        src->vtable->free(src);
}
