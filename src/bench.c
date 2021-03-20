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
#include "util.c"

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
#define DEFAULT_ITER (1<<24)
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
//====================================================================
void stx_app()
{
	stx_t s = stx_new(1);
	stx_append(s, foo);
	stx_free(s);
}

void sds_app()
{
	sds s = sdsnewlen(SDS_NOINIT, 10);
	s = sdscat(s, foo);
	sdsfree(s);
}

void stx_grow()
{
	stx_t s = stx_from("");

	for (int i=0; i<iter; ++i) 
		stx_append_alloc(&s, foo);
	
	// if (iter<10) printf("stx:'%s'\n", (char*)s);
	// stx_show(s);

	ASSERT (stx_len(s), foolen*iter);
	
	stx_free(s);
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


#define TEXTLEN 1<<8
char* sep = "a";
char* text;
int sepcnt;

void split_stx()
{
	unsigned int cnt=0;
	stx_t* parts = stx_split(text, TEXTLEN, sep, &cnt);
	
	int totlen = 0;
	for (int i = 0; i < cnt; ++i) {
		totlen += stx_len(parts[i]);
	}
	
	ASSERT (totlen+sepcnt, TEXTLEN);
}

void split_sds()
{
	int cnt=0;
	sds* parts = sdssplitlen(text, TEXTLEN, sep, strlen(sep), &cnt);
	
	int totlen = 0;
	for (int i = 0; i < cnt; ++i) {
		totlen += sdslen(parts[i]);
	}
	
	ASSERT (totlen+sepcnt, TEXTLEN);
}

//====================================================================

#define BOLD  "\033[1m"
#define RESET "\033[0m"

int main (int argc, char **argv) 
{
	if (argc > 1)
		iter = atoi(argv[1]);
	else
		iter = DEFAULT_ITER;
	
	BENCH ("SDS append", sds_app, iter);
	BENCH ("STX append", stx_app, iter);

	BENCH ("SDS grow", sds_grow, 1);
	BENCH ("STX grow", stx_grow, 1);
	
	text = rand_str(TEXTLEN);
	ASSERT (strlen(text), TEXTLEN);
	sepcnt = str_count(text,sep);

	// ASSERT (strlen(text)+sepcnt, TEXTLEN); // IF SEP = 1 char only

	BENCH ("SDS split", split_sds, iter>>6);
	BENCH ("STX split", split_stx, iter>>6);
    return 0;
}