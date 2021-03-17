#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

#include "stx.h"
#include "log.h"

#include "../sds/sds.h"
//====================================================================

#define ASSERT(a, b) { \
    if (a!=b) { \
        fprintf(stderr, "%d: assertion %s==%s failed: %s==%d %s==%d\n", \
            __LINE__, #a, #b, #a, (int)a, #b, (int)b); \
        exit(1); \
    } \
}

// 1<<20 = 1.048.576
#define ITER (1<<2)
int iter;

const char* foo = "foo";
const size_t foolen = 3;

#define BENCH(title, func, iter) \
printf ("%s (x%d) : %li ms\n", title, iter, bench(func,iter))

clock_t bench (void(*func)(void), unsigned int iter)
{
	clock_t start = clock();
	for (int i=0; i<iter; ++i) func();
	clock_t lapse = clock() - start;

	return round(1000*(float)lapse/CLOCKS_PER_SEC);
}

void stx_app()
{
	stx_t s = stx_new(1);
	stx_append(s, foo);
	// stx_free(s);
}

void stx_grow()
{
	stx_t s = stx_from("",0);

	for (int i=0; i<iter; ++i) 
		stx_append_alloc(&s, foo);
	
	// if (iter<10) printf("stx:'%s'\n", (char*)s);
	// stx_show(s);

	ASSERT (stx_len(s), foolen*iter);
	
	stx_free(s);
}

void sds_app()
{
	sds s = sdsnewlen(SDS_NOINIT, 10);
	sdscat(s, foo);
	// sdsfree(s);  // free(): double free detected in tcache 2 !!
}

void sds_grow()
{
	sds s = sdsnew("");
	
	for (int i=0; i<iter; ++i) 
		s = sdscat(s, foo);
	
	// if (iter<10) printf("s:'%s'\n", (char*)s);

	ASSERT (sdslen(s), foolen*iter);

	sdsfree(s);
}




#define BOLD  "\033[1m"
#define RESET "\033[0m"

int main (int argc, char **argv) 
{
	iter = argc ? atoi(argv[1]) : ITER;
	
	BENCH (BOLD "STX" RESET " append", stx_app, iter);
	BENCH ("SDS append", sds_app, iter);

	BENCH ("STX grow", stx_grow, 1);
	BENCH ("SDS grow", sds_grow, 1);

    return 0;
}