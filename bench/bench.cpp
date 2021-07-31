/* 
	requires: libbenchmark-dev 
	Debian/Ubuntu: sudo apt install libbenchmark-dev
*/

#include <benchmark/benchmark.h>
#include <string>
#include <array>

// ClobberMemory() : avoids optimize-out

extern "C" {
#include "sds/sds.h"
#include "../src/stx.h"
}

static std::string randStr(size_t n) 
{
	std::array<char, 4> ch = {'a', 'b', 'c', 'd'}; 
	std::string ret;
	for (size_t i = 0; i<n; ++i)
		ret.push_back(ch[i % ch.size()]);
  	return ret;
}

// ==== Init and free ==================================================

#define INIT_FREE(Type, New, Free) \
	const char* src = randStr(state.range(0)).c_str(); \
	const size_t srclen = strlen(src); \
	for (auto _ : state) { \
		Type s = New(src, srclen); \
		assert(s); \
		Free(s); \
	} \
	benchmark::ClobberMemory()

static void 
STX_from (benchmark::State& state) {
	INIT_FREE (stx_t, stx_from_len, stx_free);
}

static void 
SDS_from (benchmark::State& state) {
	INIT_FREE (sds, sdsnewlen, sdsfree);
}

// ==== Append ====================================================

static void 
STX_append (benchmark::State& state) 
{
	const char* src = randStr(state.range(0)).c_str();
	const size_t srclen = strlen(src);
	stx_t s = stx_from("");

	for (auto _ : state) {
		stx_append (&s, src, srclen);
	}
	benchmark::ClobberMemory();
	stx_free(s);
}

static void 
SDS_append (benchmark::State& state) 
{
	const char* src = randStr(state.range(0)).c_str();
	const size_t srclen = strlen(src);
	sds s = sdsnew("");

	for (auto _ : state) {
		s = sdscatlen(s, src, srclen);
	}
	benchmark::ClobberMemory();
	sdsfree(s);
}

// static void 
// std_append(benchmark::State& state) 
// {
// 	auto const src = randStr(state.range(0));
// 	std::string s;
// 	for (auto _ : state) {
// 		s += src;
// 	}
// 	benchmark::ClobberMemory();
// }

//==== Split and join =========================================

#define SPLIT_SEP "|"

#define SPLIT_INIT \
	const size_t partlen = state.range(0);\
	const size_t count = 100;\
	const std::string pat = randStr(partlen) + SPLIT_SEP;\
	std::string ssrc = "";\
	for (int i = 0; i < count; ++i)	ssrc += pat;\
	const char* src = ssrc.c_str();\
	const size_t srclen = strlen(src);\
	const size_t seplen = strlen(SPLIT_SEP);\
    int cnt = 0;

static void 
STX_split_join (benchmark::State& state) 
{
	SPLIT_INIT

    for (auto _ : state) {
	    stx_t* parts = stx_split_len (src, srclen, SPLIT_SEP, seplen, &cnt);
	    stx_t back = stx_join_len (parts, cnt, SPLIT_SEP, seplen);
		stx_list_free(parts);
	    assert (!strcmp(src,back));
	    stx_free(back);
	}

	benchmark::ClobberMemory();
}

static void 
SDS_split_join (benchmark::State& state) 
{
	SPLIT_INIT

    for (auto _ : state) {
	    sds* parts = sdssplitlen (src, srclen, SPLIT_SEP, seplen, &cnt);
	    sds back = sdsjoinsds (parts, cnt, SPLIT_SEP, seplen);
    	sdsfreesplitres(parts,cnt);
	    assert (!strcmp(src,back));
	    sdsfree(back);
	}

	benchmark::ClobberMemory();
}

//=====================================================================

#define MULT 8
#define RANGE_END 1<<15

BENCHMARK(SDS_from)->RangeMultiplier(MULT)->Range(8, RANGE_END);
BENCHMARK(STX_from)->RangeMultiplier(MULT)->Range(8, RANGE_END);

BENCHMARK(SDS_append)->RangeMultiplier(MULT)->Range(8, RANGE_END);
BENCHMARK(STX_append)->RangeMultiplier(MULT)->Range(8, RANGE_END);

BENCHMARK(SDS_split_join)->RangeMultiplier(MULT)->Range(8, RANGE_END)->Unit(benchmark::kMicrosecond);
BENCHMARK(STX_split_join)->RangeMultiplier(MULT)->Range(8, RANGE_END)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();