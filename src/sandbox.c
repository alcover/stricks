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

#define x4(s) s s s s
#define s8 "[8.....]"
#define s32 x4(s8)
#define s256 x4(s32) x4(s32)

// void split(const char* word, const char* sep, size_t n) 
// { 
//     const char* pat = str_cat(word,sep); //LOGVS(pat);
//     const char* txt = str_repeat(pat,n); //LOGVS(txt);
//     const size_t wordlen = strlen(word);
//     size_t cnt = 0;
//     stx_t* list = stx_split_len(txt, strlen(txt), sep, 1, &cnt);
//     assert(cnt==n+1);
    
//     for (size_t i = 0; i < cnt-1; ++i) {
//         stx_t s = list[i];
//         // ASSERT_PROPS (s, wordlen, wordlen, word);
//     }
//     // ASSERT_STR(list[cnt-1], "");

//     // free((char*)pat);
//     // free((char*)txt);
//     // stx_list_free(list);
// }

int main() 
{
	// split("foo","|",STX_POOL_MAX*2);


	return 0; 
}