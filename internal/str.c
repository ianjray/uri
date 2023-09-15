#include "str.h"

#include <errno.h>
#include <string.h>

int str_consume(char **s1, const char *prefix)
{
    if (!strncmp(*s1, prefix, strlen(prefix))) {
        *s1 += strlen(prefix);
        return 1;
    }

    return 0;
}

char *str_take_until_pivot(char **s1, char *pivot)
{
    if (pivot) {
        size_t n = (size_t)(pivot - *s1);
        char *taken = strndup(*s1, n);
        if (taken) {
            *s1 += n;
        }
        return taken;
    }

    return NULL;
}

char *str_take_until(char **s1, int c)
{
    return str_take_until_pivot(s1, strchr(*s1, c));
}

char *str_take_from(char *s, int c, size_t skip)
{
    char *pivot = strchr(s, c);
    if (pivot) {
        char *taken = strdup(pivot + skip);
        *pivot = '\0';
        return taken;
    }

    return NULL;
}

size_t str_begins(const char *str, const char *prefix)
{
    size_t len = strlen(prefix);
    if (!strncmp(str, prefix, len)) {
        return len;
    }

    return 0;
}

size_t str_equal(const char *str, const char *other)
{
    if (!strcmp(str, other)) {
        return strlen(other);
    }

    return 0;
}

int str_append(char *buffer, size_t cap, const char *str, size_t len)
{
    size_t size = strlen(buffer) + 1 /*NUL*/;

    if (size > cap) {
        // Already exceeded available capacity.
        errno = ENOSPC;
        return -1;
    }

    if (size + len > cap) {
        // Would exceed available capacity.
        errno = ENOSPC;
        return -1;
    }

    memcpy(&buffer[size - 1], str, len);
    buffer[size - 1 + len] = '\0';
    return 0;
}
