#include "parse.h"
#include "file.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>

static inline bool parse_peek_line_end(const char **src_p)
{
    char c = **src_p;
    return !c || c == '\n' || c == '\r';
}

static inline bool parse_line_end(const char **src_p)
{
    const char *src = *src_p;
    switch (src[0]) {
    case 0:
        return true;
    case '\n':
        return (*src_p = src + (src[1] == '\r') + 1);
    case '\r':
        return (*src_p = src + 1);
    default:
        return false;
    }
}

static inline char parse_dot(const char **src_p)
{
    const char c = **src_p;
    return c ? (*src_p += 1, c) : false;
}

static inline char parse_char(const char **src_p, char c)
{
    return c == **src_p ? (*src_p += 1, c) : false;
}

static inline char parse_range(const char **src_p, char lo, char hi)
{
    char c = **src_p;
    return (unsigned char)lo <= c && c <= (unsigned char)hi ? (*src_p += 1, c)
                                                            : false;
}

static inline char parse_set(const char **src_p, const char *set)
{
    char c = **src_p;
    for (unsigned i = 0; set[i]; i++) {
        if (c == set[i]) {
            return (*src_p += 1, c);
        }
    }
    return false;
}

static inline const char *parse_cstr(const char **src_p, const char *cstr)
{
    const char *src = *src_p;
    unsigned i;
    for (i = 0; cstr[i]; i++) {
        if (src[i] != cstr[i]) {
            return false;
        }
    }
    return (*src_p = src + i, cstr);
}

static inline const char *parse_cstr_set(const char **src_p, const char **set)
{
    for (unsigned i = 0; set[i]; i++) {
        if (parse_cstr(src_p, set[i])) {
            return set[i];
        }
    }
    return false;
}

typedef struct ucd_parse_enum {
    const char *key;
    unsigned value;
} ucd_parse_enum;

static inline const unsigned *
parse_enum(const char **src_p, const ucd_parse_enum *table)
{
    for (unsigned i = 0; table[i].key; i++) {
        if (parse_cstr(src_p, table[i].key)) {
            return &table[i].value;
        }
    }
    return false;
}

static inline bool parse_blank_opt(const char **src_p)
{
    parse_return(
        parse_star(parse_dot(src_p), parse_filter(isblank(parse_value))));
}

static inline bool debug_echo_line(const char **src_p)
{
    parse_return(parse_echo(parse_until(parse_peek_line_end(src_p))));
}

static inline bool parse_skip_ud_field(const char **src_p)
{
    parse_return(
        parse_until(
            parse_or(parse_char(src_p, ';'), parse_peek_line_end(src_p))),
        parse_opt(parse_char(src_p, ';')));
}

static inline bool parse_codepoint(const char **src_p, uint32_t *dest)
{
    uint32_t cp = 0;
    parse_return(
        parse_repeat(
            4, 6,
            parse_or(
                parse_seq(
                    parse_range(src_p, '0', '9'),
                    parse_effect(cp = (cp << 4) | (parse_value - '0'))),
                parse_seq(
                    parse_range(src_p, 'A', 'F'),
                    parse_effect(cp = (cp << 4) | (parse_value - 'A' + 10))))),
        parse_effect(*dest = cp));
}

static inline bool parse_comment(const char **src_p)
{
    parse_return(
        parse_char(src_p, '#'), parse_until(parse_peek_line_end(src_p)));
}

static inline bool parse_empty_line(const char **src_p)
{
    parse_commit(parse_blank_opt(src_p));
    parse_try(parse_peek_line_end(src_p));
    parse_return(parse_comment(src_p));
}

static inline bool parse_peek_end_of_field(const char **src_p)
{
    parse_try(parse_peek_line_end(src_p));
    parse_return(parse_peek(parse_char(src_p, '#')));
}

#define parse_field(...)                                             \
    parse_seq(                                                       \
        parse_blank_opt(src_p), __VA_ARGS__, parse_blank_opt(src_p), \
        parse_or(                                                    \
            parse_char(src_p, ';'), parse_comment(src_p),            \
            parse_peek_line_end(src_p)))

static inline bool parse_skip_field(const char **src_p)
{
    parse_return(parse_field(parse_until(parse_peek_end_of_field(src_p))));
}

static inline bool
parse_cp_range(const char **src_p, uint32_t *lo, uint32_t *hi)
{
    parse_return(
        parse_codepoint(src_p, lo),
        parse_or(
            parse_seq(parse_cstr(src_p, ".."), parse_codepoint(src_p, hi)),
            parse_effect(*hi = *lo)));
}

static inline bool parse_codepoint_name(const char **src_p)
{
    unsigned vs = 0;
    parse_return(
        parse_dot(src_p), parse_filter(isupper(parse_value)),
        parse_star(
            parse_opt(parse_char(src_p, ' ')), parse_dot(src_p),
            parse_filter(
                isupper(parse_value) || isdigit(parse_value) ||
                parse_value == '-')));
}

static inline bool parse_property_name(const char **src_p)
{
    parse_return(
        parse_dot(src_p), parse_filter(isupper(parse_value)),
        parse_star(
            parse_dot(src_p),
            parse_filter(isalpha(parse_value) || parse_value == '_')));
}

static inline bool parse_combining_class(const char **src_p)
{
    unsigned cc = 0;
    parse_return(
        parse_repeat(
            1, 3, parse_range(src_p, '0', '9'),
            parse_effect(cc = (cc * 10) + parse_value - '0')),
        parse_filter(cc < 255));
}

static inline bool parse_int(const char **src_p)
{
    parse_return(
        parse_plus(parse_dot(src_p), parse_filter(isdigit(parse_value))));
}

static inline bool parse_float(const char **src_p)
{
    parse_return(
        parse_opt(parse_char(src_p, '-')),
        parse_plus(parse_dot(src_p), parse_filter(isdigit(parse_value))),
        parse_char(src_p, '.'),
        parse_plus(parse_dot(src_p), parse_filter(isdigit(parse_value))));
}

static inline bool parse_ratio(const char **src_p)
{
    parse_return(
        parse_opt(parse_char(src_p, '-')), parse_int(src_p),
        parse_char(src_p, '/'), parse_int(src_p));
}

static inline bool parse_line(const char **src_p)
{
    uint32_t lo, hi;
    // Comment Line
    parse_try(parse_empty_line(src_p));
    // Property Flag
    parse_try(
        parse_field(parse_cp_range(src_p, &lo, &hi)),
        parse_field(parse_property_name(src_p)));
    // Grapheme Test
    parse_try(parse_field(
        parse_cstr(src_p, "÷ "), parse_codepoint(src_p, &lo),
        parse_star(
            parse_or(parse_cstr(src_p, " ÷ "), parse_cstr(src_p, " × ")),
            parse_codepoint(src_p, &lo)),
        parse_cstr(src_p, " ÷")));
    // Line Break Test
    parse_try(parse_field(
        parse_cstr(src_p, "× "), parse_codepoint(src_p, &lo),
        parse_star(
            parse_or(parse_cstr(src_p, " ÷ "), parse_cstr(src_p, " × ")),
            parse_codepoint(src_p, &lo)),
        parse_cstr(src_p, " ÷")));
    // Emoji Variation Sequence
    parse_try(
        parse_field(
            parse_codepoint(src_p, &lo), parse_char(src_p, ' '),
            parse_codepoint(src_p, &hi)),
        parse_field(parse_or(
            parse_cstr(src_p, "emoji style"), parse_cstr(src_p, "text style"))),
        parse_field(parse_blank_opt(src_p)));
    // Derived Combining Class
    parse_try(
        parse_field(parse_cp_range(src_p, &lo, &hi)),
        parse_field(parse_combining_class(src_p)));
    // Derived Line Break
    parse_try(
        parse_field(parse_cp_range(src_p, &lo, &hi)),
        parse_field(
            parse_dot(src_p), parse_filter(isupper(parse_value)),
            parse_dot(src_p),
            parse_filter(isupper(parse_value) || isdigit(parse_value))));
    // Derived Name
    parse_try(
        parse_field(parse_codepoint(src_p, &lo)),
        parse_field(parse_codepoint_name(src_p)));
    parse_try(
        parse_field(parse_cp_range(src_p, &lo, &hi)),
        parse_field(parse_codepoint_name(src_p), parse_char(src_p, '*')));
    // Derived Numeric Values
    parse_try(
        parse_field(parse_cp_range(src_p, &lo, &hi)),
        parse_field(parse_float(src_p)), parse_field(parse_blank_opt(src_p)),
        parse_field(parse_ratio(src_p)));
    parse_try(
        parse_field(parse_cp_range(src_p, &lo, &hi)),
        parse_field(parse_float(src_p)), parse_field(parse_blank_opt(src_p)),
        parse_field(parse_int(src_p)));

    return false;
}

bool load_file(const char *path)
{
    if (!path) {
        return false;
    }

    const char *file = file_load(path);
    if (!file) {
        return false;
    }

    bool retval = true;
    const char *src = file;
    while (*src) {
        const char *trace = src;
        if (!parse_line(&trace) || !parse_line_end(&trace)) {
            retval = false;
            trace = src;
            debug_echo_line(&trace);
        }
        src = trace;
    }
    file_free(file);

    if (!retval) {
        printf("Failed File: %s\n", path);
    }

    return retval;
}
