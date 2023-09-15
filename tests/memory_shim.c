#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void * (*g_libc_malloc)(size_t);
static void * (*g_libc_realloc)(void *, size_t);
static bool has_registered;
static unsigned count;
static unsigned cap = UINT_MAX;

static void tidy(void)
{
    printf("X_ALLOC_COUNT:%u\n", count);
}

/// Shim for simulating out-of-memory.
void *malloc(size_t size)
{
    if (!g_libc_malloc) {
        g_libc_malloc = (void * (*)(size_t))dlsym(RTLD_NEXT, "malloc");

        char *e = getenv("X_ALLOC_CAP");
        if (e) {
            cap = atoi(e);
        } else if (!has_registered) {
            has_registered = true;
            atexit(tidy);
        }
    }

    count++;

    if (cap > 0) {
        cap--;
        return g_libc_malloc(size);
    }

    errno = ENOMEM;
    return NULL;
}

/// Shim for simulating out-of-memory.
void *realloc(void *ptr, size_t size)
{
    if (!g_libc_realloc) {
        g_libc_realloc = (void * (*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");

        char *e = getenv("X_ALLOC_CAP");
        if (e) {
            cap = atoi(e);
        } else if (!has_registered) {
            has_registered = true;
            atexit(tidy);
        }
    }

    count++;

    if (cap > 0) {
        cap--;
        return g_libc_realloc(ptr, size);
    }

    errno = ENOMEM;
    return NULL;
}

/// Shim for simulating out-of-memory.
void *calloc(size_t count, size_t size)
{
    void *p = malloc(count * size);
    if (p) {
        memset(p, 0, count * size);
    }
    return p;
}

/// Shim for simulating out-of-memory.
char *strdup(const char *str)
{
    char *p = malloc(strlen(str) + 1);
    if (p) {
        strcpy(p, str);
    }
    return p;
}
