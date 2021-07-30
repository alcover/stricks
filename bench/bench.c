/*
Stricks - Managed C strings library
Copyright (C) 2021 - Francois Alcover <francois[@]alcover.fr>
NO WARRANTY EXPRESSED OR IMPLIED
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <tgmath.h> // lroundl (long double)

#include "../src/stx.h"
#define ENABLE_LOG
#include "../src/log.h"
#include "../src/util.c"
#include "../src/test_strings.c"

#include "sds/sds.h"

//==============================================================================
#define uint unsigned long

#define FOR(i,n) for(uint i=0; i<(n); ++i)

#define BENCHBEG \
	int counter = 0; \
	clock_t start = clock();

#define BENCHEND(lib) \
	long double time = (long double)(clock()-start); \
	time = 1000*time/CLOCKS_PER_SEC; \
	time = lroundl(time); \
	LOG("%9s  %.0Lf ms", lib, time); \
	++counter; \
	assert(counter>0);

#define SECTION(title) LOG("\n" title "\n" "---------------");

//==============================================================================

void STX_from (const char* src, uint n)
{
	const size_t srclen = strlen(src);
	stx_t* arr = malloc(n * sizeof(*arr));
	
	BENCHBEG
		FOR(i,n) arr[i] = stx_from_len(src,srclen);
		FOR(i,n) stx_free(arr[i]);
	BENCHEND("Stricks")
	
	free(arr);
}

void SDS_from (const char* src, uint n)
{
	const size_t srclen = strlen(src);
	sds* arr = malloc(n * sizeof(*arr));
	
	BENCHBEG
		FOR(i,n) arr[i] = sdsnewlen(src,srclen);
		FOR(i,n) sdsfree(arr[i]);
	BENCHEND("SDS")
	
	free(arr);
}

void u_new (const char* w, uint n) 
{
	LOG ("%zu bytes strings :", strlen(w));
	SDS_from (w, n);
	STX_from (w, n);
}

void new_free()
{
	SECTION("init and free")
	u_new (W8,  10000000);
	// u_new(W64,5000000);
	u_new (W256, 4000000);
}

//==============================================================================

void STX_append (const char* src, uint n)
{
	const size_t srclen = strlen(src);
	stx_t s = stx_from("");
	
	BENCHBEG
		FOR(i,n) stx_append (&s, src, srclen);
	BENCHEND("Stricks")
	
	assert(stx_len(s)==srclen*n);
	stx_free(s);
}

void SDS_append (const char* src, uint n)
{
	const size_t srclen = strlen(src);
	sds s = sdsnew("");

	BENCHBEG
		FOR(i,n) s = sdscatlen (s, src, srclen);
	BENCHEND("SDS")
	
	assert(sdslen(s)==srclen*n);
	sdsfree(s);
}

void u_app (const char* w, uint n) 
{
	LOG ("%zu bytes strings :", strlen(w));
	SDS_append (w, n);
	STX_append (w, n);
}

void append()
{
	SECTION("append")
	u_app (W8,	50000000);
	u_app (W256, 5000000);
}

//==============================================================================

#define FMT "%s %s %d"
#define ARG "foo", "bar", 10

void STX_append_fmt (uint iter)
{
    const int fmtlen = snprintf(NULL, 0, FMT, ARG);

	stx_t s = stx_from("");
	
	BENCHBEG
		FOR(i,iter) stx_append_fmt (&s, FMT, ARG);
	BENCHEND("Stricks")
	
	assert (stx_len(s)==fmtlen*iter);
	stx_free(s);
}


void SDS_append_fmt (uint iter)
{
	const int fmtlen = snprintf(NULL, 0, FMT, ARG);

	sds s = sdsnew("");

	BENCHBEG
		FOR(i,iter) s = sdscatprintf (s, FMT, ARG);
	BENCHEND("SDS")
	
	assert (sdslen(s)==fmtlen*iter);
	sdsfree(s);
}

void append_fmt()
{
	SECTION("append format")
	SDS_append_fmt (5000000);
	STX_append_fmt (5000000);
}
//==============================================================================

void STX_split_join (const char* src, const char* sep)
{
	const size_t srclen = strlen(src);
	const size_t seplen = strlen(sep);
    int cnt = 0;

    BENCHBEG
	    stx_t* parts = stx_split_len (src, srclen, sep, seplen, &cnt);
	    stx_t back = stx_join_len (parts, cnt, sep, seplen);
    	stx_list_free(parts);
	BENCHEND("Stricks")

    assert (!strcmp(src,back));
    stx_free(back);
}

void SDS_split_join (const char* src, const char* sep)
{
	const size_t srclen = strlen(src);
	const size_t seplen = strlen(sep);
    int cnt = 0;

    BENCHBEG
	    sds* parts = sdssplitlen (src, srclen, sep, seplen, &cnt);
	    sds back = sdsjoinsds (parts, cnt, sep, seplen);
    	sdsfreesplitres(parts,cnt);
	BENCHEND("SDS")

    assert (!strcmp(src,back));
    sdsfree(back);
}

void u_join (const char* word, const char* sep, uint n)
{
    const char* pat = str_cat(word,sep);
    const char* src = str_repeat(pat,n);
	LOG ("%zu bytes strings :", strlen(word));

	SDS_split_join (src, sep); 
	STX_split_join (src, sep);

	free((void*)src);
	free((void*)pat);
}

void split_join()
{
	SECTION("split and join")
	u_join (W8,	  "|", 5000000);
	u_join (W256, "|", 500000);
}


//==============================================================================
int main () 
{
	new_free();
	append();
	append_fmt();
	split_join();

	return 0;
}