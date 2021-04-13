
// dev box = Intel Core i3 (2011)

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
            __LINE__, #a, #b, #a, (int)(a), #b, (int)b); \
        exit(1); \
    } \
}

#define FOR(i,n) for (int i = 0; i < n; ++i)

const char* foo = "foo";
const size_t foolen = 3;

//====================================================================

void STX_append_dyn (const char* word, int n)
{
	stx_t s = stx_from("");
	FOR(i,n) 
		stx_append_alloc (&s, foo);
	ASSERT (stx_len(s), foolen*n);
	stx_free(s);
}

void SDS_append_dyn (const char* word, int n)
{
	sds s = sdsnew("");
	FOR(i,n) 
		s = sdscat (s, foo);
	ASSERT (sdslen(s), foolen*n);
	sdsfree(s);
}


void STX_new (size_t cap, int n)
{
	stx_t* arr = malloc(n * sizeof(*arr));

	FOR(i,n)
		arr[i] = stx_new(cap);
	FOR(i,n)
		stx_free(arr[i]);
	
	free(arr);
}

void STX_from_len (const char* src, size_t len, int n)
{
	stx_t* arr = malloc(n * sizeof(*arr));

	FOR(i,n)
		arr[i] = stx_from_len(src,len);
	FOR(i,n)
		stx_free(arr[i]);
	
	free(arr);
}

void SDS_from_len (const char* src, size_t len, int n)
{
	sds* arr = malloc(n * sizeof(*arr));

	FOR(i,n)
		arr[i] = sdsnewlen(src,len);
	FOR(i,n)
		sdsfree(arr[i]);
	
	free(arr);
}



void STX_split (char* text, size_t textlen, char* sep)
{
	unsigned int nparts = 0;
	stx_t* parts = stx_split(text, textlen, sep, &nparts);
	
	size_t partslen = 0;
	FOR(i,nparts) partslen += stx_len(parts[i]);
	
	ASSERT (partslen + (nparts-1)*strlen(sep), textlen);
}

void SDS_split(char* text, size_t textlen, char* sep)
{
	int nparts = 0;
	sds* parts = sdssplitlen(text, textlen, sep, strlen(sep), &nparts);
	
	size_t partslen = 0;
	FOR(i,nparts) partslen += sdslen(parts[i]);
	
	ASSERT (partslen + (nparts-1)*strlen(sep), textlen);
}


#define BOLD  "\033[1m"
#define RESET "\033[0m"

#define BENCH(func, args, times, bold) { \
	printf ("%s (x%d) : ", #func, times);\
	clock_t start = clock();\
	for (int i=0; i<times; ++i) func args;\
	clock_t ticks = clock() - start;\
	unsigned int mstime = round(1000*(float)ticks/CLOCKS_PER_SEC); \
	if(bold) printf(BOLD);\
	printf ("%d ms\n", mstime); \
	if(bold) printf(RESET);\
}

// 1<<20 = 1M
#define DEFAULT_ORDER 24

//====================================================================

int main (int argc, char **argv) {

int order;
if (argc > 1)
	order = atoi(argv[1]);
else
	order = 0;
int N = 2<<(20+order);

BENCH (SDS_from_len, (foo,foolen,N), 1, 0);
BENCH (STX_from_len, (foo,foolen,N), 1, 1);

BENCH (SDS_append_dyn, (foo,N), 1, 0);
BENCH (STX_append_dyn, (foo,N), 1, 1);

char* text;
size_t textlen;

textlen = N*4;
text = rand_str(textlen, "abcd,");
// ASSERT (strlen(text), TEXTLEN); 
// sepcnt = str_count(text,sep);
BENCH (SDS_split, (text,textlen,","), 1, 0);
BENCH (STX_split, (text,textlen,","), 1, 1);


return 0;}