#ifndef UCD_GEN_PARSE_H
#define UCD_GEN_PARSE_H

#include "file.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

bool load_file(const char *path);

#define LPAREN (
#define RPAREN )
#define PARENS ()
#define EAT(...)

#define OPT_SWITCH(...)                                           \
    EVAL1(__VA_OPT__(EAT LPAREN)                                  \
              OPT_SWITCH__ELSE __VA_OPT__(RPAREN OPT_SWITCH__IF))
#define OPT_SWITCH__IF(...)   __VA_ARGS__ EAT
#define OPT_SWITCH__ELSE(...) EVAL1

#define EVAL        EVAL16
#define EVAL16(...) EVAL8(EVAL8(__VA_ARGS__))
#define EVAL8(...)  EVAL4(EVAL4(__VA_ARGS__))
#define EVAL4(...)  EVAL2(EVAL2(__VA_ARGS__))
#define EVAL2(...)  EVAL1(EVAL1(__VA_ARGS__))
#define EVAL1(...)  __VA_ARGS__

#define parse_return(...)                 \
    intptr_t parse_value = true;          \
    parse_value = parse_seq(__VA_ARGS__); \
    return parse_value

#define parse_return_ptr(...)             \
    intptr_t parse_value = true;          \
    parse_value = parse_seq(__VA_ARGS__); \
    return (void *)parse_value

#define parse_commit(...)                     \
    do {                                      \
        intptr_t parse_value = true;          \
        parse_value = parse_seq(__VA_ARGS__); \
        if (!parse_value)                     \
            return false;                     \
    } while (0)

#define parse_try(...)                                   \
    for (typeof(*src_p) parse_mark = *src_p; parse_mark; \
         *src_p = parse_mark, parse_mark = false) {      \
        intptr_t parse_value = true;                     \
        parse_value = parse_seq(__VA_ARGS__);            \
        if (parse_value)                                 \
            return parse_value;                          \
    }

#define parse_try_ptr(...)                               \
    for (typeof(*src_p) parse_mark = *src_p; parse_mark; \
         *src_p = parse_mark, parse_mark = false) {      \
        intptr_t parse_value = true;                     \
        parse_value = parse_seq(__VA_ARGS__);            \
        if (parse_value)                                 \
            return (void *)parse_value;                  \
    }

#define parse_seq(step, ...)                                   \
    (intptr_t)step __VA_OPT__(; PARSE_SEQ__BLOCK(__VA_ARGS__))
#define PARSE_SEQ__BLOCK(...)                \
    do {                                     \
        EVAL(PARSE_SEQ__CHECK(__VA_ARGS__)); \
    } while (0)
#define PARSE_SEQ__CHECK(...)    \
    if (!parse_value)            \
        break;                   \
    PARSE_SEQ__STEP(__VA_ARGS__)
#define PARSE_SEQ__STEP(step, ...)                                       \
    parse_value =                                                        \
        (intptr_t)step __VA_OPT__(; PARSE_SEQ__CONT PARENS(__VA_ARGS__))
#define PARSE_SEQ__CONT() PARSE_SEQ__CHECK

#define parse_or(alt, ...)                                                    \
    OPT_SWITCH(__VA_ARGS__)(PARSE_OR__BLOCK(alt, __VA_ARGS__))((intptr_t)alt)
#define PARSE_OR__BLOCK(...)                \
    parse_value;                            \
    do {                                    \
        typeof(*src_p) parse_mark = *src_p; \
        intptr_t parse_hold = parse_value;  \
        EVAL(PARSE_OR__ALT(__VA_ARGS__));   \
    } while (0)
#define PARSE_OR__ALT(alt, ...)                                            \
    parse_value = (intptr_t)alt __VA_OPT__(; PARSE_OR__CHECK(__VA_ARGS__))
#define PARSE_OR__CHECK(...)           \
    if (parse_value)                   \
        break;                         \
    *src_p = parse_mark;               \
    parse_value = parse_hold;          \
    PARSE_OR__CONT PARENS(__VA_ARGS__)
#define PARSE_OR__CONT() PARSE_OR__ALT

#define parse_peek(...)                       \
    parse_value;                              \
    do {                                      \
        typeof(*src_p) parse_mark = *src_p;   \
        intptr_t parse_hold = parse_value;    \
        parse_value = parse_seq(__VA_ARGS__); \
        if (!parse_value)                     \
            break;                            \
        *src_p = parse_mark;                  \
        parse_value = parse_hold;             \
    } while (0)

#define parse_not(...)                        \
    parse_value;                              \
    do {                                      \
        typeof(*src_p) parse_mark = *src_p;   \
        intptr_t parse_hold = parse_value;    \
        parse_value = parse_seq(__VA_ARGS__); \
        if (parse_value) {                    \
            parse_value = false;              \
            break;                            \
        }                                     \
        *src_p = parse_mark;                  \
        parse_value = parse_hold;             \
    } while (0)

#define parse_opt(...)                        \
    parse_value;                              \
    do {                                      \
        typeof(*src_p) parse_mark = *src_p;   \
        intptr_t parse_hold = parse_value;    \
        parse_value = parse_seq(__VA_ARGS__); \
        if (parse_value)                      \
            break;                            \
        *src_p = parse_mark;                  \
        parse_value = parse_hold;             \
    } while (0)

#define parse_star(...)                       \
    parse_value;                              \
    for (;;) {                                \
        typeof(*src_p) parse_mark = *src_p;   \
        intptr_t parse_hold = parse_value;    \
        parse_value = parse_seq(__VA_ARGS__); \
        if (!parse_value) {                   \
            *src_p = parse_mark;              \
            parse_value = parse_hold;         \
            break;                            \
        }                                     \
    }

#define parse_plus(...) parse_seq(__VA_ARGS__, parse_star(__VA_ARGS__))

#define parse_count(n, ...)                                          \
    parse_value;                                                     \
    for (unsigned parse__n = (n), parse__i = 0; parse__i < parse__n; \
         parse__i++) {                                               \
        parse_value = parse_seq(__VA_ARGS__);                        \
        if (!parse_value)                                            \
            break;                                                   \
    }

#define parse_repeat(lo, hi, ...)                                 \
    parse_value;                                                  \
    do {                                                          \
        unsigned parse__lo = (lo);                                \
        unsigned parse__hi = (hi);                                \
        parse_value = parse_count(parse__lo, __VA_ARGS__);        \
        if (!parse_value)                                         \
            break;                                                \
        for (unsigned parse__i = parse__lo; parse__i < parse__hi; \
             parse__i++) {                                        \
            typeof(*src_p) parse_mark = *src_p;                   \
            intptr_t parse_hold = parse_value;                    \
            parse_value = parse_seq(__VA_ARGS__);                 \
            if (!parse_value) {                                   \
                *src_p = parse_mark;                              \
                parse_value = parse_hold;                         \
                break;                                            \
            }                                                     \
        }                                                         \
    } while (0)

#define parse_until(...) parse_star(parse_not(__VA_ARGS__), parse_dot(src_p))

#define parse_filter(cond) ((cond) ? parse_value : false)

#define parse_effect(stmnt, ...)                               \
    parse_value;                                               \
    stmnt __VA_OPT__(; EVAL(PARSE_EFFECT__STMNT(__VA_ARGS__)))
#define PARSE_EFFECT__STMNT(stmnt, ...)                        \
    stmnt __VA_OPT__(; PARSE_EFFECT__CONT PARENS(__VA_ARGS__))
#define PARSE_EFFECT__CONT() PARSE_EFFECT__STMNT

#define parse_echo(...)                                                    \
    parse_value;                                                           \
    do {                                                                   \
        typeof(*src_p) parse_mark = *src_p;                                \
        parse_value = parse_seq(__VA_ARGS__);                              \
        if (parse_value)                                                   \
            printf("%.*s\n", (unsigned)(*src_p - parse_mark), parse_mark); \
    } while (0)

#endif
