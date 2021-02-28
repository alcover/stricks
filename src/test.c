#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#include "stx.h"
#include "log.h"

int main() {
{
	stx_t s = stx_new(6);
	stx_show(s);
	stx_append_count(s, "foo", 0);
	stx_show(s); 
	printf("%zu %zu\n", stx_cap(s), stx_len(s));
}

{
	stx_t s = stx_new(257);
	stx_show(s);
	stx_append_count(s, "foo", 0);
	stx_show(s); 
	printf("%zu %zu\n", stx_cap(s), stx_len(s));
}
    return 0;
}