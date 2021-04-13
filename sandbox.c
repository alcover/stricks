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

#include "src/stx.h"
#include "src/log.h"
#include "src/util.c"

#define x4(s) s s s s
#define s8 "[8.....]"
#define s32 x4(s8)
#define s256 x4(s32) x4(s32)

int main() {

/********************************************************************/

stx_t s = stx_from_len("Stricks", 10);
stx_show(s); 

stx_t strick = stx_from("Stricks");
stx_append_alloc (&strick, " are treats!");        

printf("%s\n", strick);

// stx_t s = stx_new(8);
// ((char*)s)[0] = 'a';
// stx_show(s);

// stx_t s;

// s = stx_from("abc", 0);
// stx_show(s);

// s = stx_from(s256, 0);
// stx_show(s);

// const char* text = rand_str(1<<8);
// unsigned int l=0;
// stx_t* list = stx_split(text, "a", &l);

// LOG(text); puts("\n");
// for (int i = 0; i < l; ++i)
// 	printf("%s\n", list[i]);


/********************************************************************/
return 0; }