#include "uri.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    struct uri *u;
    char *str;
    int r;

    r = uri_new("scheme://user:password@host:123/path/to?query?q#fragment?q", &u);
    if (r < 0) {
        exit(0);
    }

    r = uri_str(u, &str);
    if (r == 0) {
        assert(!strcmp("scheme://user:password@host:123/path/to?query?q#fragment?q", str));
        free(str);
    }

    {
        struct uri *u2;

        r = uri_dup(u, &u2);
        if (r < 0) {
            goto out;
        }

        uri_delete(u2);
    }

    r = uri_set(u, "scheme2://user2:password2@host2:456/path2/to2?query2?q#fragment2?q");
    if (r < 0) {
        goto out;
    }

    r = uri_str(u, &str);
    if (r == 0) {
        assert(!strcmp("scheme2://user2:password2@host2:456/path2/to2?query2?q#fragment2?q", str));
        free(str);
    }

    assert(!strcmp("scheme2", uri_scheme(u)));
    assert(!strcmp("user2:password2", uri_userinfo(u)));
    assert(!strcmp("host2", uri_host(u)));
    assert(!strcmp("456", uri_port(u)));
    assert(!strcmp("/path2/to2", uri_path(u)));
    assert(!strcmp("query2?q", uri_query(u)));
    assert(!strcmp("fragment2?q", uri_fragment(u)));

    r = uri_scheme_set(u, "other");
    if (r < 0) {
        assert(!strcmp("scheme2", uri_scheme(u)));
    } else {
        assert(!strcmp("other", uri_scheme(u)));
    }

    r = uri_scheme_set(u, NULL);
    assert(!uri_scheme(u));

    r = uri_str(u, &str);
    if (r == 0) {
        assert(!strcmp("//user2:password2@host2:456/path2/to2?query2?q#fragment2?q", str));
        free(str);
    }

    r = uri_userinfo_set(u, "u3:p3");
    if (r < 0) {
        assert(!strcmp("user2:password2", uri_userinfo(u)));
    } else {
        assert(!strcmp("u3:p3", uri_userinfo(u)));
    }

    r = uri_port_set(u, "8");
    if (r < 0) {
        assert(!strcmp("456", uri_port(u)));
    } else {
        assert(!strcmp("8", uri_port(u)));
    }

    r = uri_port_set(u, NULL);
    assert(!uri_port(u));

    r = uri_str(u, &str);
    if (r == 0) {
        assert(!strcmp("//u3:p3@host2/path2/to2?query2?q#fragment2?q", str));
        free(str);
    }

    r = uri_path_set(u, "there");
    if (r < 0) {
        assert(!strcmp("/path2/to2", uri_path(u)));
    } else {
        assert(!strcmp("/path2/there", uri_path(u)));

        r = uri_path_set(u, "/here");
        if (r < 0) {
            assert(!strcmp("/path2/there", uri_path(u)));
        } else {
            assert(!strcmp("/here", uri_path(u)));
        }
    }

    r = uri_query_set(u, "Query");
    if (r < 0) {
        assert(!strcmp("query2?q", uri_query(u)));
    } else {
        assert(!strcmp("Query", uri_query(u)));
    }

    r = uri_query_set(u, NULL);
    assert(!uri_query(u));

    r = uri_str(u, &str);
    if (r == 0) {
        assert(!strcmp("//u3:p3@host2/here#fragment2?q", str));
        free(str);
    }

    r = uri_fragment_set(u, "Fragment");
    if (r < 0) {
        assert(!strcmp("fragment2?q", uri_fragment(u)));
    } else {
        assert(!strcmp("Fragment", uri_fragment(u)));
    }

    r = uri_fragment_set(u, NULL);
    assert(!uri_fragment(u));

    r = uri_str(u, &str);
    if (r == 0) {
        assert(!strcmp("//u3:p3@host2/here", str));
        free(str);
    }

    r = uri_fragment_set(u, "?Fragment");
    if (r < 0) {
        assert(!uri_fragment(u));
    } else {
        assert(!strcmp("?Fragment", uri_fragment(u)));
    }

    r = uri_str(u, &str);
    if (r == 0) {
        assert(!strcmp("//u3:p3@host2/here#?Fragment", str));
        free(str);
    }

    r = uri_query_set(u, "Query#");
    if (r < 0) {
        assert(!uri_query(u));
    } else {
        assert(!strcmp("Query#", uri_query(u)));
    }

    r = uri_str(u, &str);
    if (r == 0) {
        assert(!strcmp("//u3:p3@host2/here?Query%23#?Fragment", str));
        free(str);
    }

    r = uri_port_set(u, "123");
    if (r < 0) {
        assert(!uri_port(u));
    } else {
        assert(!strcmp("123", uri_port(u)));
    }

    r = uri_str(u, &str);
    if (r == 0) {
        assert(!strcmp("//u3:p3@host2:123/here?Query%23#?Fragment", str));
        free(str);
    }

    r = uri_scheme_set(u, "scheme");
    if (r < 0) {
        assert(!uri_scheme(u));
    } else {
        assert(!strcmp("scheme", uri_scheme(u)));
    }

    r = uri_str(u, &str);
    if (r == 0) {
        assert(!strcmp("scheme://u3:p3@host2:123/here?Query%23#?Fragment", str));
        free(str);
    }

out:
    uri_delete(u);
}
