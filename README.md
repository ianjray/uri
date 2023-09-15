# uri
An [RFC 3986](https://datatracker.ietf.org/doc/html/rfc3986]) URI object model.

Features include:.
* Strict validation on input
* Controlled mutation via setters
* Canonical, security-aware serialization

The public API is defined in [uri.h](uri.h).

## Security

All input strings **must** be valid UTF-8.

* Malformed UTF-8 is rejected.
* Unicode C0 and C1 code points (U+0000-U+001F, U+007F, U+0080–U+009F) are also rejected on input.
* As a convenience, UTF-8 characters are *automatically* percent-encoded by `uri_new`, `uri_set`, and component setters.
* Characters outside the allowed set (or that would be interpreted as a delimiter) **must** be percent-encoded by the caller.

All output strings are guaranteed to be RFC compliant ASCII and guaranteed to **not** contain ASCII C0 control characters (U+0000–U+001F, U+007F) either *literally* or via *percent-encoding*.

* **Warning** failing to validate and properly handle URI components can lead to security vulnerabilities, including injection attacks.

## Conformity

URIs are re-written according to RFC guidance:

1. Drop redundant percent-encodings §2.3.
2. Use uppercase percent-encodings §3.1.
3. Use lowercase scheme names §3.1.
4. Use lowercase host names §3.2.2.
5. Remove dot segments §5.2.4.

This libary performs the following *additional* steps:

1. Drop leading zeroes in port numbers.
2. Enforce range of port numbers from 0..65535.

## Non-Conformity

1. The host component is tokenized **only**.

## Example

```c
#include <assert.h>
#include <liburi/uri.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    struct uri *u;
    char *str;

    u = uri_new("http://user@example.com:80/path/sub?query#fragment");

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

    uri_path_set(u, "dir/file");
    assert(!strcmp(uri_path(u),     "/path/dir/file"));

    str = uri_str(u);
    assert(!strcmp(str, "https://user@example.com/path/dir/file?query#fragment"));
    free(str);

    uri_port_set(u, "00443");
    assert(!strcmp(uri_port(u),     "443"));

    uri_query_set(u, "?");
    assert(!strcmp(uri_query(u),    "?"));

    uri_fragment_set(u, "%23");
    assert(!strcmp(uri_fragment(u), "%23"));

    str = uri_str(u);
    assert(!strcmp(str, "https://user@example.com:443/path/dir/file??#%23"));
    free(str);

    assert(0 == uri_set(u, "../new"));

    str = uri_str(u);
    assert(!strcmp(str, "https://user@example.com:443/path/new"));
    free(str);

    uri_delete(u);
}
```

## Installation

```bash
./configure
make
sudo make install
```

## Requirements

- C99 or later
- POSIX-compatible system

## Thread Safety

This library is **not** thread-safe.
