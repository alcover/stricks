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

{ 
stx_t s = stx_from("abc"); 
stx_append(&s, "def", 3); //-> 6 
stx_dbg(s); 
}

{
stx_t foo = stx_new(32);
stx_append_fmt (&foo, "%s has %d apples", "Mary", 10);
stx_dbg(foo);	
}

return 0; 
}