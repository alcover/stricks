/* 
	Try code against the API
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#include "stx.h"
// #include "log.h"
#include "util.c"
#define ERR(...) fprintf(stderr, "\x1b[33m" "line %d: %s\n" "\033[0m", __LINE__ , __VA_ARGS__)

int main() {
ERR("popo:%d", 11);
{
char* src = "foo,bar";
char* sep = ",";
int count = 0;
stx_t* parts = stx_split(src, sep, &count);
stx_t joined = stx_join (parts, count, sep);
stx_dbg(joined);

}

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