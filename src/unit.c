#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#include "stx.h"
#include "util.c"
//==============================================================================

#define ASSERT_INT(a, b) { if ( ((int)(a)) != ((int)(b)) ) { \
fprintf(stderr, "%d: %s:%d != %s:%d\n", __LINE__, #a, (int)a, #b, (int)b); \
exit(1);}}

#define ASSERT_STR(a, b) { if (strcmp(a, b)) {\
fprintf(stderr, "%d: %s:'%s' != %s:'%s'\n", __LINE__, #a, a, #b, b); \
exit(1);}}

#define assert_props(s, cap, len, data) {\
    ASSERT_INT (stx_cap(s), cap);\
    ASSERT_INT (stx_len(s), len);\
    ASSERT_STR ((s), data);\
}

//==============================================================================

#define CAP 100  
#define SMALL_MAX 255
#define FOO "foo"
#define BAR "bar"
#define SEP "|"

const char* foo = "foo";
const size_t foolen = 3;
const char* bar = "bar";
const size_t barlen = 3;
const char* foobar = "foobar";
const size_t foobarlen = 6;

#define l64 "abbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbc"
const char* big = l64 l64 l64 l64;
#define biglen 256
static_assert (biglen > SMALL_MAX, "biglen");

//==============================================================================

#define u_new(cap) {\
    stx_t s = stx_new(cap);\
    assert_props (s, cap, 0, ""); \
    stx_free(s);\
}

void new() 
{ 
    u_new (0); 
    u_new (1); 
    u_new (CAP); 
    u_new (SMALL_MAX-1); 
    u_new (SMALL_MAX); 
    u_new (SMALL_MAX+1); 
    u_new (UINT32_MAX); 
}

//==============================================================================

#define u_from(src, expcap, explen, expdata){\
    stx_t s = stx_from(src);\
    assert_props (s, expcap, explen, expdata);\
    stx_free(s);\
}

void from() 
{    
    // u_from (NULL,   0, 0, ""); 
    u_from ("",     0, 0, ""); 
    u_from (foo,    foolen, foolen, foo); 
    u_from (big,    biglen, biglen, big); 
}

#define u_from_len(src, srclen, expcap, explen, expdata){\
    stx_t s = stx_from_len(src,srclen);\
    assert_props (s, expcap, explen, expdata);\
    stx_free(s);\
}

void from_len() 
{
    // u_from_len (NULL, 0,       0, 0, ""); 
    // u_from_len (NULL, CAP,     0, 0, ""); 
    u_from_len ("", 0,         0, 0, ""); 
    u_from_len (foo, 0,         0, 0, ""); 
    u_from_len (foo, foolen-1,  foolen-1, foolen-1, "fo");     
    u_from_len (foo, foolen,    foolen, foolen, foo); 
    u_from_len (foo, foolen+1,  foolen+1, foolen+1, foo); 
    u_from_len (big, 64,        64, 64, l64); 
    u_from_len (big, biglen,    biglen, biglen, big); 
}

//==============================================================================

#define u_dup(src) {\
    stx_t s = stx_from(src);\
    stx_t d = stx_dup(s);\
    size_t len = stx_len(s);\
    assert_props (d, len, len, src); \
    stx_free(s); \
    stx_free(d);\
}

void dup() {
    // u_dup(NULL);
    u_dup("");
    u_dup(foo);
    u_dup(big);
}

//==============================================================================
#define u_rsznew(initcap, newcap){\
    stx_t s = stx_new(initcap);\
    stx_resize(&s, newcap);\
    assert_props (s, newcap, 0, "");\
    stx_free(s);}

#define u_rszfrom(src,newcap, explen, expdata){\
    stx_t s = stx_from(src);\
    stx_resize(&s, newcap);\
    assert_props (s, newcap, explen, expdata);\
    stx_free(s);}

void resize()
{
    u_rsznew (0, 0);
    u_rsznew (0, SMALL_MAX);
    u_rsznew (0, SMALL_MAX+1);

    u_rsznew (CAP, 0);
    u_rsznew (CAP, CAP);
    u_rsznew (CAP, SMALL_MAX);
    u_rsznew (CAP, SMALL_MAX+1);

    u_rsznew (SMALL_MAX, 0);
    u_rsznew (SMALL_MAX, SMALL_MAX-1);
    u_rsznew (SMALL_MAX, SMALL_MAX);
    u_rsznew (SMALL_MAX, SMALL_MAX+1);

    u_rsznew (SMALL_MAX+1,0);
    u_rsznew (SMALL_MAX+1,SMALL_MAX);
    u_rsznew (SMALL_MAX+1,SMALL_MAX+2);

    u_rszfrom ("", 0,   0, "");
    u_rszfrom ("", CAP,   0, "");
    u_rszfrom ("", SMALL_MAX+1,   0, "");

    u_rszfrom (foo, 0,  0,"");
    u_rszfrom (foo, 2, 2, "fo");
    u_rszfrom (foo, SMALL_MAX, foolen, foo);
    u_rszfrom (foo, SMALL_MAX+1, foolen, foo);

    u_rszfrom (big, 0,  0,"");
    u_rszfrom (big, 64, 64, l64);
    u_rszfrom (big, biglen+1, biglen, big);

}

//==============================================================================

void append()
{
    #define INIT(cap, src, len, exprc, expdata) { \
        stx_t s = stx_new(cap); \
        int rc = stx_append (&s, src, len); \
        ASSERT_INT (rc, (int)exprc);   \
        ASSERT_STR (s, expdata);        \
        stx_free(s);                                        \
    }
    
    // no room
    INIT (0, foo, foolen,   foolen, foo);
    // under
    INIT (foolen-1, foo, foolen, foolen, foo);
    // exact
    INIT (foolen  , foo, foolen, foolen, foo);
    // over
    INIT (foolen+1, foo, foolen, foolen, foo);

    // big
    INIT (0, big, biglen, biglen, big);
    INIT (foolen  , big, biglen, biglen, big);
    INIT (biglen+1, big, biglen, biglen, big);

    // cut
    INIT (0, foo, foolen-1, foolen-1, "fo");
    INIT (foolen-1, foo, foolen-1, foolen-1, "fo");
    INIT (foolen  , foo, foolen-1, foolen-1, "fo");
    INIT (foolen+1, foo, foolen-1, foolen-1, "fo");
    #undef INIT
}

void append_strict()
{
    #define INIT(cap, src, len,   exprc, explen, expdata) { \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_strict (s, src, len);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        assert_props (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    // null: user concern
    // INIT (foolen,    NULL, foolen,    0,     0, "");    
    // no room
    INIT (0, foo, foolen, -foolen, 0, "");
    INIT (0, big, biglen, -biglen, 0, "");
    // under
    INIT (foolen-1, foo, foolen,    -foolen, 0, "");
    INIT (biglen-1, big, biglen,    -biglen, 0, "");
    // exact
    INIT (foolen, foo, foolen,    foolen, foolen, foo);
    INIT (biglen, big, biglen,    biglen, biglen, big);
    // over
    INIT (foolen+1, foo, foolen,    foolen, foolen, foo);
    INIT (biglen+1, big, biglen,    biglen, biglen, big);
    // cut
    INIT (foolen, foo, foolen-1,  foolen-1,   foolen-1,   "fo");
    INIT (biglen, big, 64,    64, 64, l64);
    #undef INIT

    #define MORE(cap, src, len, exprc, explen, expdata) { \
        stx_t s = stx_new(cap);  \
        stx_append_strict (s, foo, foolen);       \
        int rc = stx_append_strict (s, src, len);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        assert_props (s, cap, explen, expdata); \
        stx_free(s); \
    }
    
    // todo big
    // no room
    MORE (foolen, bar, barlen, -foobarlen, foolen, foo);
    // under
    MORE (foolen+1, bar, barlen, -foobarlen, foolen, foo);
    // exact
    MORE (foobarlen, bar, barlen, foobarlen, foobarlen, foobar);
    // over
    MORE (foobarlen+1, bar, barlen, foobarlen, foobarlen, foobar);
    // cut
    MORE (foobarlen, bar, barlen-1, foobarlen-1, foobarlen-1, "fooba");
    #undef MORE
}

//==============================================================================

void append_fmt()
{
    #define INIT(cap, fmt, src, exprc, expcap, explen, expdata) { \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_fmt (&s, fmt, src);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        assert_props (s, expcap, explen, expdata); \
        stx_free(s);                                        \
    }

    INIT (foolen,   "%s", "",   0,      foolen,     0,      "");
    INIT (foolen,   "%s", foo,  foolen, foolen,     foolen, foo);
    INIT (foolen+1, "%s", foo,  foolen, foolen+1,   foolen, foo);
    INIT (foolen-1, "%s", foo,  foolen, foolen,     foolen, foo);
    #undef INIT

    #define MORE(cap, fmt, src, exprc, expcap, explen, expdata) { \
        stx_t s = stx_new(cap);                            \
        stx_append_strict (s, foo, foolen);                 \
        int rc = stx_append_fmt (&s, fmt, src);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        assert_props (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    MORE (foobarlen, "%s", bar, foobarlen, foobarlen, foobarlen, foobar);
    #undef MORE
}


void append_fmt_strict()
{
    #define INIT(cap, fmt, src, exprc, explen, expdata) { \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_fmt_strict (s, fmt, src);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        assert_props (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    INIT (foolen,   "%s", foo,  foolen,  foolen, foo);
    INIT (foolen+1, "%s", foo,  foolen,  foolen, foo);
    INIT (foolen-1, "%s", foo,  -foolen, 0,      "");
    #undef INIT

    #define MIX(cap, fmt, src1, src2, exprc, explen, expdata) {  \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_fmt_strict (s, fmt, src1, src2);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        assert_props (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    MIX (foobarlen,   "%s%s", foo, bar, foobarlen, foobarlen, foobar);
    MIX (foobarlen+1, "%s%s", foo, bar, foobarlen, foobarlen, foobar);
    MIX (foobarlen-1, "%s%s", foo, bar, -foobarlen, 0, "");
    #undef MIX

    #define MORE(cap, fmt, src, exprc, explen, expdata) {   \
        stx_t s = stx_new(cap);                            \
        stx_append_strict (s, foo, foolen);                 \
        int rc = stx_append_fmt_strict (s, fmt, src);                 \
        ASSERT_INT (rc, (int)exprc);                                \
        assert_props (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    MORE (foobarlen, "%s", bar, foobarlen, foobarlen, foobar);
    #undef MORE
}

//==============================================================================

typedef stx_t*(*splitter)(const char*, size_t, const char*, size_t, int*);

void split_cust(
splitter fun, const char* str, const char* sep, int expc, char* exps[])
{
    int cnt = 0;
    stx_t* list = fun(str, strlen(str), sep, strlen(sep), &cnt);
    stx_t* l = list;
    stx_t s;
    size_t i=0;

    ASSERT_INT(cnt,expc);

    while ((s = *l++)) {
        const char* exp = exps[i++];
        const size_t explen = strlen(exp);
        assert_props (s, explen, explen, exp);
    }

    stx_list_free(list);
}


// (foo, |, 3) => "foo|foo|foo|" => 4 : foo,foo,foo,""
void split_pat (splitter fun, const char* word, const char* sep, size_t n) 
{ 
    const char* pat = str_cat(word,sep); //LOGVS(pat);
    const char* txt = str_repeat(pat,n); //LOGVS(txt);
    const size_t wordlen = strlen(word);
    int cnt = 0;

    stx_t* list = fun(txt, strlen(txt), sep, strlen(sep), &cnt);

    ASSERT_INT(cnt,n+1);
    
    for (int i = 0; i < cnt-1; ++i) {
        stx_t s = list[i];
        assert_props (s, wordlen, wordlen, word);
    }
    ASSERT_STR(list[cnt-1], "");

    free((char*)pat);
    free((char*)txt);
    stx_list_free(list);
}

void u_split(splitter fun)
{
    // split_cust (fun, foo,                 "",     0,  (char*[]){""});
    split_cust (fun, FOO,                SEP,    1,  (char*[]){FOO});
    split_cust (fun, FOO SEP,            SEP,    2,  (char*[]){FOO,""});
    split_cust (fun, FOO SEP BAR,        SEP,    2,  (char*[]){FOO,BAR});
    split_cust (fun, FOO SEP BAR SEP,    SEP,    3,  (char*[]){FOO,BAR,""});
    split_cust (fun, FOO SEP SEP BAR,    SEP,    3,  (char*[]){FOO,"",BAR});
    split_cust (fun, "baaaad",    "aa",    3,  (char*[]){"b","","d"});

    split_pat (fun, FOO, SEP, 6);
    split_pat (fun, FOO, SEP, STX_STACK_MAX-1);
    split_pat (fun, FOO, SEP, STX_STACK_MAX);
    split_pat (fun, FOO, SEP, STX_STACK_MAX+1);
    split_pat (fun, FOO, SEP, STX_POOL_MAX-1);
    split_pat (fun, FOO, SEP, STX_POOL_MAX);
    split_pat (fun, FOO, SEP, STX_POOL_MAX+1);

    // split_pat (fun, FOO, SEP, 20000000);
}

void split() {u_split(stx_split_len);}
// void split_fast() {u_split(stx_split_fast);}

//==============================================================================

#define u_splitjoin(src,sep,seplen) { \
    int count; \
    stx_t* list = stx_split(src, sep, &count); \
    stx_t joined = stx_join_len(list, count, sep, seplen); \
    ASSERT_STR(joined,src); \
    assert(stx_len(joined) == strlen(src)); \
    stx_list_free(list);\
}

void join() 
{ 
    // u_splitjoin (NULL, NULL);
    // u_splitjoin ("", NULL);
    // u_splitjoin ("", "");
    // u_splitjoin (FOO, "");  
    u_splitjoin (FOO, SEP, 1); 
    u_splitjoin (FOO SEP FOO, SEP, 1);
    u_splitjoin (SEP FOO SEP FOO, SEP, 1);
    u_splitjoin (FOO SEP FOO SEP, SEP, 1);
    u_splitjoin (FOO SEP SEP FOO, SEP, 1);

    u_splitjoin (FOO BAR FOO, BAR, barlen);
}

//==============================================================================
void reset()
{
    stx_t s = stx_new(CAP);
    stx_reset(s);
    assert_props (s, CAP, 0, ""); 
    stx_append (&s, foo, foolen);
    stx_reset(s);
    assert_props (s, CAP, 0, ""); 
    stx_free(s);
}

void adjust()
{
    stx_t s = stx_new(CAP);
    stx_append (&s, foo, foolen);
    ((char*)s)[foolen-1] = 0;
    stx_adjust(s);
    assert_props (s, CAP, foolen-1, "fo"); 
    stx_free(s);
}

void equal()
{
    stx_t a = stx_new(3);
    stx_t b = stx_new(4);
    assert(stx_equal(a,b));

    stx_append (&a, foo, foolen);
    stx_append (&b, foo, foolen);
    assert(stx_equal(a,b));

    stx_append (&b, "o", 1);
    assert(!stx_equal(a,b));

    stx_free(a);
    stx_free(b);    
}

#define u_trim(src, expcap, explen) {\
    stx_t s = stx_from(src);\
    stx_trim(s);\
    assert_props(s, expcap, explen, foo);\
    stx_free(s);\
}

void trim()
{
    u_trim (foo, foolen, foolen)
    u_trim (" foo", 4, foolen)
    u_trim ("foo ", 4, foolen)
    u_trim (" foo ", 5, foolen)
}

//==============================================================================

void story()
{
    stx_t a = stx_new(foolen);
    stx_append_strict (a, big, biglen); //nop
    stx_append_strict (a, foo, foolen); //a==foo

    stx_t b = stx_from(bar);
    stx_t c = stx_dup(b); //c==bar
    size_t clen = stx_len(c);

    stx_append(&a, c, clen); //a==foobar

    stx_reset(b);
    stx_resize(&b,foobarlen);
    stx_append_fmt (&b, "%s%s", foo, c); //b==foobar

    assert(stx_equal(a,b));
}

//==============================================================================

#define run(name) { \
    printf("%s ", #name); fflush(stdout); \
    name(); \
    puts("OK"); \
}

int main()
{
    assert(strlen(big)==biglen);
    
    run(new);
    run(from);
    run(from_len);
    run(dup);
    run(join);
    run(split);
    // run(split_fast);

    run(append);
    run(append_strict);
    run(append_fmt);
    run(append_fmt_strict);
    
    // run(free_);
    run(resize);
    run(reset);
    run(adjust);
    run(trim);
    
    run(equal);
    
    run(story);

    printf ("unit tests OK\n");
    return 0;
}