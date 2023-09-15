#include "rfc_charset.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>

int main(void)
{
    assert( is_userinfo(':'));
    assert(!is_userinfo('@'));
    assert(!is_userinfo('^'));

    assert( is_path(':'));
    assert( is_path('@'));
    assert( is_path('/'));
    assert(!is_path('^'));

    printf("Testing completed.\n");
    return 0;
}
