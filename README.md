# uri
URI Normalizer

An [RFC 3986](https://datatracker.ietf.org/doc/html/rfc3986]) URI normalizer.

The public API is defined in [uri.h](uri.h).

## Correctness

The URI components are validated, normalized, and joined according to the RFC.

- Note: the host component is **only** tokenized (checked for valid characters and percent-encoding).

- Callers **must** parse tokens for correctness **before** using them.

## Example

```c
#include <assert.h>
#include <liburi/uri.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    struct uri *u = NULL;
    char *str = NULL;

    assert(0 == uri_new("http://user@example.com:80/path/sub?query#fragment", &u));

    assert(!strcmp(uri_scheme(u),   "http"));
    assert(!strcmp(uri_userinfo(u), "user"));
    assert(!strcmp(uri_host(u),     "example.com"));
    assert(!strcmp(uri_port(u),     "80"));
    assert(!strcmp(uri_path(u),     "/path/sub"));
    assert(!strcmp(uri_query(u),    "query"));
    assert(!strcmp(uri_fragment(u), "fragment"));

    uri_scheme_set(u, "https");
    assert(!strcmp(uri_scheme(u),   "https"));

    uri_port_set(u, NULL);

    uri_path_set(u, "other");
    assert(!strcmp(uri_path(u),     "/path/other"));

    assert(0 == uri_str(u, &str));
    assert(!strcmp(str, "https://user@example.com/path/other?query#fragment"));
    free(str);

    uri_port_set(u, "443");

    uri_query_set(u, "?");
    assert(!strcmp(uri_query(u),    "?"));

    uri_fragment_set(u, "#");
    assert(!strcmp(uri_fragment(u), "#"));

    assert(0 == uri_str(u, &str));
    assert(!strcmp(str, "https://user@example.com:443/path/other??#%23"));
    free(str);

    assert(0 == uri_set(u, "../new"));

    assert(0 == uri_str(u, &str));
    assert(!strcmp(str, "https://user@example.com:443/new"));
    free(str);

    uri_delete(u);
}
```
