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

#define X16(s) #s #s #s #s #s #s #s #s #s #s #s #s #s #s #s #s 
#define BIG X16(aaaabbbbccccdddd)
#define biglen 256
const char* big = BIG;
const char* huge = BIG BIG BIG BIG BIG BIG BIG BIG BIG BIG BIG BIG BIG BIG BIG BIG;


int main() {

int a = max(2,1+3);
LOGVI(a);
// unsigned int count;
// stx_t* results = stx_split("baaaad", "aa", &count);

// for (int i = 0; i < count; ++i) {
//     printf("%s\n", results[i]);
// }

	// stx_t s = stx_from("foo, bar", 0);
	// unsigned cnt = 0;

	// stx_t* list = stx_split(s, ", ", &cnt);

	// for (int i = 0; i < cnt; ++i) {
	//     // printf("%s\n", list[i]);
	//     stx_show(list[i]);
	// }
	 
	// {
	// 	stx_t s = stx_new(6);
	// 	stx_show(s);
	// 	stx_append_count(s, "foo", 0);
	// 	stx_show(s); 
	// 	printf("%zu %zu\n", stx_cap(s), stx_len(s));
	// }

	// {
	// 	stx_t s = stx_new(300);
	// 	stx_show(s);
	// 	stx_append_count_alloc(&s, "foo", 0);
	// 	stx_show(s); 
	// 	printf("%zu %zu\n", stx_cap(s), stx_len(s));
	// }

	// {
	// 	stx_t s = stx_from("foo");
	// 	stx_show(s);
	// 	stx_resize(&s, 300);
	// 	stx_show(s); 
	// 	printf("%zu %zu\n", stx_cap(s), stx_len(s));
	// }
	// {
	// 	stx_t s = stx_new(3);
	// 	stx_show(s);
	// 	stx_append_count_alloc(&s, big, 0);
	// 	stx_show(s); 
	// 	printf("%zu %zu\n", stx_cap(s), stx_len(s));
	// }


    return 0;
}