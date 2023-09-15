#include "memory_shim.h"

#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void * (*g_libc_malloc)(size_t);
static void * (*g_libc_calloc)(size_t, size_t);
static char * (*g_libc_strdup)(const char *);
static char * (*g_libc_strndup)(const char *, size_t);
static void * (*g_libc_realloc)(void *, size_t);

static unsigned count_;
static unsigned n_;

void memory_shim_reset(void)
{
    count_ = 0;
    n_ = 0;
}

void memory_shim_fail_at(unsigned nth)
{
    count_ = 0;
    n_ = nth;
}

unsigned memory_shim_count_get(void)
{
    return count_;
}

static bool allow(void)
{
    return ++count_ != n_;
}

void *malloc(size_t size)
{
    if (!g_libc_malloc) {
        g_libc_malloc = (void * (*)(size_t))dlsym(RTLD_NEXT, "malloc");
    }

    if (allow()) {
        return g_libc_malloc(size);
    }

    errno = ENOMEM;
    return NULL;
}

void *calloc(size_t count, size_t size)
{
    if (!g_libc_calloc) {
        g_libc_calloc = (void * (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
    }

    if (allow()) {
        return g_libc_calloc(count, size);
    }

    errno = ENOMEM;
    return NULL;
}

char *strdup(const char *str)
{
    if (!g_libc_strdup) {
        g_libc_strdup = (char * (*)(const char *))dlsym(RTLD_NEXT, "strdup");
    }

    if (allow()) {
        return g_libc_strdup(str);
    }

    errno = ENOMEM;
    return NULL;
}

char *strndup(const char *str, size_t n)
{
    if (!g_libc_strndup) {
        g_libc_strndup = (char * (*)(const char *, size_t))dlsym(RTLD_NEXT, "strndup");
    }

    if (allow()) {
        return g_libc_strndup(str, n);
    }

    errno = ENOMEM;
    return NULL;
}

void *realloc(void *ptr, size_t size)
{
    if (!g_libc_realloc) {
        g_libc_realloc = (void * (*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");
    }

    if (allow()) {
        return g_libc_realloc(ptr, size);
    }

    errno = ENOMEM;
    return NULL;
}
