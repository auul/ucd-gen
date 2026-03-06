#include "error.h"
#include "file.h"
#include "parse.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define len_of(array) (sizeof(array) / sizeof(*(array)))

static char str_buf[0x80000];
size_t str_idx;

static const char *dir_stack[16];
size_t dir_idx;

static const char *file_stack[512];
size_t file_idx;

static inline char *str_bump(const char *src)
{
    size_t len = strlen(src);
    char *dest = str_buf + str_idx;
    str_idx += len;
    assert(str_idx < len_of(str_buf));
    memcpy(dest, src, len);
    return dest;
}

static inline void str_finish(void)
{
    str_buf[str_idx++] = 0;
}

static inline const char *push_path(const char *root, const char *name)
{
    if (root) {
        const char *path = str_bump(root);
        str_bump(UCD_PATH_SLASH);
        str_bump(name);
        str_finish();
        return path;
    } else {
        const char *path = str_bump(name);
        str_finish();
        return path;
    }
}

static inline void push_dir(const char *root, const char *name)
{
    assert(dir_idx < len_of(dir_stack));
    dir_stack[dir_idx++] = push_path(root, name);
}

static inline void push_file(const char *root, const char *name)
{
    assert(file_idx < len_of(file_stack));
    file_stack[file_idx++] = push_path(root, name);
}

static inline const char *pop_dir(void)
{
    if (!dir_idx) {
        return NULL;
    }
    dir_idx--;
    return dir_stack[dir_idx];
}

static inline const char *pop_file(void)
{
    if (!file_idx) {
        return NULL;
    }
    file_idx--;
    return file_stack[file_idx];
}

static inline bool is_entry_dir(const struct dirent *entry)
{
    return entry->d_type == DT_DIR && strcmp(entry->d_name, "..") &&
           strcmp(entry->d_name, ".");
}

static inline bool is_data_file(const char *filename)
{
    size_t len = strlen(filename);
    return len > 4 && !strcmp(filename + len - 4, ".txt") &&
           strcmp(filename, "ReadMe.txt");
}

void dir_list(const char *root)
{
    push_dir(NULL, root);
    while ((root = pop_dir())) {
        DIR *d = opendir(root);
        if (!d) {
            error_sys(
                UCD_ERROR_ABORT, errno, "Could not open directory '%s'", root);
            continue;
        }

        for (struct dirent *entry = readdir(d); entry; entry = readdir(d)) {
            if (is_entry_dir(entry)) {
                push_dir(root, entry->d_name);
            } else if (is_data_file(entry->d_name)) {
                push_file(root, entry->d_name);
            }
        }
        closedir(d);
    }
}

int main(void)
{
    dir_list("data");
    while (load_file(pop_file()))
        ;
    return 0;
}
