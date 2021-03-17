#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#include "stx.h"
#include "log.h"
#include "util.c"

#define x4(s) s s s s

#define s8 "[8.....]"
#define s32 x4(s8)
#define s256 x4(s32) x4(s32)

int main() {

	stx_t s;

	s = stx_from("abc", 0);
	stx_show(s);

	s = stx_from(s256, 0);
	stx_show(s);


    return 0;
}