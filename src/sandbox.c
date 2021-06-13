/* 
	Scratch pad - test code against the API
*/

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


int main() {

stx_t s = stx_from("foo");
stx_dbg(s); 


return 0; 
}