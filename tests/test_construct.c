#include "support.h"
#include "uri.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HAS_SCHEME        0x001  // "scheme:"
#define HAS_AUTH          0x002  // "//"
#define HAS_USERINFO      0x004  // "user:pass"
#define HAS_USERINFO_SEP  0x008  // "@"
#define HAS_HOST          0x010  // "host"
#define HAS_PORT_SEP      0x020  // ":"
#define HAS_PORT          0x040  // 0..65535
#define HAS_PATH_ABS      0x080  // "/path"
#define HAS_PATH_REL      0x100  // "path"
#define HAS_QUERY         0x200  // ?query
#define HAS_FRAGMENT      0x400  // #fragment

static bool is_valid(int x)
{
    // The empty string.
    if (x == 0) {
        return false;
    }

    // Authority sub-components require HAS_AUTH.
    if ((x & (HAS_USERINFO_SEP | HAS_USERINFO | HAS_HOST | HAS_PORT_SEP | HAS_PORT )) && !(x & HAS_AUTH)) {
        return false;
    }

    // Data requires respective separator.
    if ((x & HAS_USERINFO) && !(x & HAS_USERINFO_SEP)) {
        return false;
    }

    if ((x & HAS_PORT) && !(x & HAS_PORT_SEP)) {
        return false;
    }

    // Zero or one path.
    if ((x & HAS_PATH_ABS) && (x & HAS_PATH_REL)) {
        return false;
    }

    // If Authority is present, path must be absolute or empty.
    if ((x & HAS_AUTH) && (x & HAS_PATH_REL)) {
        return false;
    }

    return true;
}

static size_t counter = 0;

static void gen(int x, const char *host)
{
    char input[1024];
    char expected[1024];

    input[0] = 0;
    expected[0] = 0;

    if (x & HAS_SCHEME) {
        strcat(input, "scHEme:");
        strcat(expected, "scheme:");
    }

    if (x & HAS_AUTH) {
        strcat(input, "//");
        strcat(expected, "//");
    }

    if (x & HAS_USERINFO) {
        strcat(input, "user:pass");
        strcat(expected, "user:pass");
    }

    if (x & HAS_USERINFO_SEP) {
        strcat(input, "@");
        strcat(expected, "@");
    }

    if (x & HAS_HOST) {
        strcat(input, host);
        strcat(expected, host);
    }

    if (x & HAS_PORT_SEP) {
        strcat(input, ":");
        strcat(expected, ":");
    }

    if (x & HAS_PORT) {
        strcat(input, "00000080");
        strcat(expected, "80");
    }

    if (x & HAS_PATH_ABS) {
        strcat(input, "/path");
        strcat(expected, "/path");

    } else if (x & HAS_PATH_REL) {
        strcat(input, "path");
        strcat(expected, "path");
    }

    if (x & HAS_QUERY) {
        strcat(input, "?query%41");
        strcat(expected, "?queryA");
    }

    if (x & HAS_FRAGMENT) {
        strcat(input, "#fragment%23");
        strcat(expected, "#fragment%23");
    }

    struct uri *u = uri_new(input);
    assert(u);

    char *str = uri_str(u);
    string_must_match(input, STRINGIFY(__LINE__), expected, str);
    free(str);

    if (x & HAS_SCHEME) {
        assert(!strcmp(uri_scheme(u), "scheme"));
    }

    if (x & HAS_USERINFO) {
        assert(!strcmp(uri_userinfo(u), "user:pass"));
    }

    if (x & HAS_HOST) {
        assert(!strcmp(uri_host(u), host));
    }

    if (x & HAS_PORT) {
        assert(!strcmp(uri_port(u), "80"));
    }

    if (x & HAS_PATH_ABS) {
        assert(!strcmp(uri_path(u), "/path"));

    } else if (x & HAS_PATH_REL) {
        assert(!strcmp(uri_path(u), "path"));
    }

    if (x & HAS_QUERY) {
        assert(!strcmp(uri_query(u), "queryA"));
    }

    if (x & HAS_FRAGMENT) {
        assert(!strcmp(uri_fragment(u), "fragment%23"));
    }

    uri_delete(u);
    counter++;
}

int main(void)
{
    // Iterate over every valid bit pattern.
    for (int x = 0x000; x < 0x800; ++x) {
        if (is_valid(x)) {
            if (x & HAS_HOST) {
                gen(x, "host");
                gen(x, "192.168.120.1");
                gen(x, "[2001:db8::1]");

            } else {
                gen(x, NULL);
            }
        }
    }

    printf("%zu combinations of URI components tested.\n", counter);
    printf("Testing completed.\n");
    return 0;
}
