#include "str.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *safe_strdup(const char *str)
{
    return str ? strdup(str) : NULL;
}

char *str_iterate(char *iter, bool (*first)(int), bool (*next)(int))
{
    if (iter) {
        if (first(*iter)) {
            while (next(*iter)) {
                iter++;
            }
            return iter;
        }
    }
    return NULL;
}

void str_reverse_iterate_digits(char *str, char **iter)
{
    char *p = strchr(str, 0);
    while (p > str && isdigit(p[-1])) {
        p--;
    }
    *iter = p;
}

bool str_reverse_iterate_one(char *str, char **iter, int c)
{
    if ((*iter) > str && (*iter)[-1] == c) {
        (*iter)--;
        return true;
    }
    return false;
}

char *str_split_left(char *str, char *iter)
{
    char *lhs = NULL;
    if (str && iter) {
        lhs = strndup(str, (size_t)(iter - str));
        if (lhs) {
            // Move remaining suffix to beginning of @c str.
            memmove(str, iter, strlen(iter) + 1 /*NUL*/);
        }
    }
    return lhs;
}

bool str_prefix_delete(char *str, const char *seq)
{
    if (str && seq) {
        if (!strncmp(str, seq, strlen(seq))) {
            char *rhs = str + strlen(seq);
            // Move remaining suffix to beginning of @c str.
            memmove(str, rhs, strlen(rhs) + 1 /*NUL*/);
            return true;
        }
    }
    return false;
}

bool str_delete(char *str, const char *pattern)
{
    if (str && pattern) {
        if (!strcmp(str, pattern)) {
            *str = 0;
            return true;
        }
    }
    return false;
}

char *str_split(char *str)
{
    if (str && *str) {
        *str = 0;
        return strdup(str + 1);
    }
    return NULL;
}

char *str_suffix_detach(char *str, int c)
{
    return str_split(str ? strchr(str, c) : NULL);
}

char *str_suffix_detach_inclusive(char *str, int c)
{
    char *rhs = strchr(str, c);
    if (rhs) {
        char *ret = strdup(rhs);
        *rhs = 0;
        return ret;
    }
    return NULL;
}

void str_append(char **str, const char *suffix)
{
    *str = (char *)realloc(*str, strlen(*str) + strlen(suffix) + 1 /*NUL*/);
    if (*str) {
        char *eos = strchr(*str, 0);
        memcpy(eos, suffix, strlen(suffix));
        eos[strlen(suffix)] = 0;
    }
}

char *str_lowercase(char *str)
{
    if (str) {
        for (char *p = str; *p; ++p) {
            *p = (char)tolower(*p);
        }
    }
    return str;
}

char *str_remove_first_path_segment(char *str)
{
    char *eos = strchr(str, '/');
    if (eos) {
        eos++;
    } else {
        eos = strchr(str, 0);
    }
    return str_split_left(str, eos);
}

char *str_filter_out(char *str, int c)
{
    if (str) {
        for (;;) {
            char *p = strchr(str, c);
            if (!p) {
                break;
            }
            memmove(p, p + 1, strlen(p + 1) + 1 /*NUL*/);
        }
    }
    return str;
}
