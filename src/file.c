#include "file.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void *error_msg(FILE *f, const char *path, int errnum)
{
    fprintf(
        stderr, "ucd-gen: Failed to open file '%s': %s", path,
        strerror(errnum));
    if (f) {
        fclose(f);
    }
    return false;
}

static inline bool calc_file_len(size_t *dest, FILE *f)
{
    long len;
    return !fseek(f, 0, SEEK_END) && (len = ftell(f)) >= 0 &&
           !fseek(f, 0, SEEK_SET) && (*dest = len, true);
}

const char *file_load(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return error_msg(f, path, errno);
    }

    size_t len = 0;
    if (!calc_file_len(&len, f)) {
        return error_msg(f, path, errno);
    }

    char *text = malloc(len + 1);
    if (!text) {
        return error_msg(f, path, errno);
    } else if (fread(text, 1, len, f) != len) {
        return error_msg(f, path, EBADF);
    }
    text[len] = 0;
    fclose(f);

    return text;
}

void file_free(const char *file)
{
    if (file) {
        free((void *)file);
    }
}
