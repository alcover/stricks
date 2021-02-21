/*
	Testing snippets
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#define STX_SHORT_NAMES
#include "stx.h"

#define PAGE_SZ 128

int main() {

	stx_t s = stx_new(3);
	int rc = stx_cat(s, "foobar"); // -> -6
	if (rc<0) stx_resize(&s, -rc);
	stx_cat(s, "foobar");
	stx_show(s); 

    return 0;
}