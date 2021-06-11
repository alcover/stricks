#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

#include "stx.h"
#include "../sds/sds.h"
#define ENABLE_LOG
#include "log.h"
#include "util.c"

//==============================================================================
// 1000*avg/CLOCKS_PER_SEC : imprecise
#define bench(msg, fun, arg, iter) {\
	clock_t start = clock();\
	for (int i=0;i<iter;++i) fun arg;\
	long double time = (long double)(clock()-start); \
	long double avg = time/iter;\
	LOG("%9s  %.2Lf", msg, avg);\
}

#define ARRLEN(a) (sizeof(a)/sizeof(a[0]))
#define FOR(i,n) for(int i=0; i<(n); ++i)
#define FORV(v,...) { \
int _list[] = {__VA_ARGS__}; \
int _listlen = sizeof(_list)/sizeof(_list[0]); \
for (int _i=0; _i<_listlen; ++_i) { \
	int v = _list[_i];	
#define FORVEND }}

#define RUNBEG(title) LOG("\n" title "\n" "---------------"); counter = 0;
#define RUNEND assert(counter>0);

#define WORD "hello"
#define SEP ", "
#define PAT WORD SEP
const char* foo = "foo";
const size_t foolen = 3;
// const char* sep = ",";
// const size_t seplen = 1;

// smth to assert to avoid optimized-out runs
size_t counter = 0; 
//==============================================================================

void STX_from (const char* src, size_t srclen, int n)
{
	stx_t* arr = malloc(n * sizeof(*arr));
	FOR(i,n) arr[i] = stx_from_len(src,srclen);
	FOR(i,n) stx_free(arr[i]);
	free(arr);
	++counter;
}

void SDS_from (const char* src, size_t srclen, int n)
{
	sds* arr = malloc(n * sizeof(*arr));
	FOR(i,n) arr[i] = sdsnewlen(src,srclen);
	FOR(i,n) sdsfree(arr[i]);
	free(arr);
	++counter;
}

void new_free()
{
	RUNBEG("new+free")
	FORV (n, 10, 100, 1000)
		int iter = 10000/n;
		// LOG("%d parts (x%d) :", n, iter);
		LOG("%d parts :", n);
		bench("SDS", 		SDS_from, (foo,foolen,n), iter);
		bench("Stricks", 	STX_from, (foo,foolen,n), iter);
	FORVEND
	RUNEND
}
//==============================================================================

void STX_append_dyn (int n)
{
	stx_t s = stx_from("");
	FOR(i,n) stx_append(&s, foo, foolen);
	assert(stx_len(s)==foolen*n);
	stx_free(s);
	++counter;
}

void SDS_append_dyn (int n)
{
	sds s = sdsnew("");
	FOR(i,n) s = sdscat(s, foo);
	assert(sdslen(s)==foolen*n);
	sdsfree(s);
	++counter;
}

void append()
{
	RUNBEG("append")
	FORV (n, 10, 100, 1000)
		int iter = 10000/n;
		// LOG("%d parts (x%d) :", n, iter);
		LOG("%d parts :", n);
		bench("SDS", 	 SDS_append_dyn, (n), iter);
		bench("Stricks", STX_append_dyn, (n), iter);
	FORVEND
	RUNEND
}
//==============================================================================

void STX_split (const char* src, size_t srclen, const char* sep, size_t seplen)
{
    size_t cnt = 0;
    stx_t* list = stx_split_len(src, srclen, sep, seplen, &cnt);
    stx_list_free(list);
    ++counter;
}

void SDS_split (const char* src, size_t srclen, const char* sep, size_t seplen)
{
    size_t cnt = 0;
    sds* list = sdssplitlen(src, srclen, sep, seplen, (int*)(&cnt));
    sdsfreesplitres(list,cnt);
    ++counter;
}

void split()
{
	RUNBEG("split")
	FORV (n, 50, 5000, 5000000)
		int iter = 5000000/n+1;
	    const char* src = str_repeat(PAT,n);
	    const size_t srclen = strlen(src);
	    const size_t seplen = strlen(SEP);
		// LOG("%d parts (x%d) :", n, iter);
		LOG("%d parts :", n);

		bench ("SDS", 		SDS_split, (src,srclen,SEP, seplen), iter);  
		bench ("Stricks", 	STX_split, (src,srclen,SEP, seplen), iter);  
		
		// bench ("Stricks-fast", 	STX_split_fast, nc, iter);  
	FORVEND
	RUNEND
}

//==============================================================================
int main (int argc, char **argv) 
{
	LOG("Time in clock ticks");

	new_free();
	append();
	split();

	return 0;
}