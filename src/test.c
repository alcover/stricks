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



void scb (const char* s, size_t len, void* ctx){
	printf("'%.*s' %zu\n", (int)len, s, len);
}

int main() {


	str_split ("de, la, balle", ", ", scb, NULL);	
	 
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