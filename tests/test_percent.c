#include "percent.h"
#include "support.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void normalize_pass(const char *input, const char *output)
{
    char *actual = strdup(input);
    assert(0 == percent_encoded_string_normalize(actual));
    string_must_match(__FUNCTION__, input, output, actual);
    free(actual);
}

static void normalize_fail(const char *input)
{
    char *actual = strdup(input);
    errno = 0;
    assert(-1 == percent_encoded_string_normalize(actual));
    assert(EINVAL == errno);
    free(actual);
}

static void test_percent_encoded_string_normalize(void)
{
    normalize_fail("%");
    normalize_fail("%F");
    normalize_fail("%FG");

    normalize_pass("", "");
    normalize_pass("A", "A");
    normalize_pass("%41", "A");
    normalize_pass("%2a", "%2A");
    normalize_pass("%2A", "%2A");
    normalize_pass("%41%c3%bc%42%4A", "A%C3%BCBJ");
}

int main(void)
{
    test_percent_encoded_string_normalize();

    printf("Testing completed.\n");
    return 0;
}
