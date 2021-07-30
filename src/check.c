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
#include <errno.h>
#include <stdarg.h>

#include "stx.h"
#include "util.c"
#include "test_strings.c"
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

//==============================================================================

static void u_new (size_t cap) 
{            
    stx_t s = stx_new(cap);     
    if (!s) {printf("stx_new(%zu) failed.\n", cap); return;}

    if (cap) assert (stx_cap(s) >= cap);   
    ASSERT_INT (stx_len(s), 0);
    ASSERT_STR (s, "");            
    
    stx_free(s);        
}

static void new() 
{ 
    u_new (0);
    u_new (1);
    u_new (SMALL_MAX/2);
    u_new (SMALL_MAX-1);
    u_new (SMALL_MAX);
    u_new (SMALL_MAX+1);
    u_new (1024);
    u_new (1024*1024); 
    u_new (32*1024*1024); 
}

//==============================================================================

static void u_from (size_t srclen)
{
    const char* src = str_nchar('a', srclen);
    if (!src) {printf("create src(%zu) failed.\n", srclen); return;}
    stx_t s = stx_from(src);
    
    assert (stx_cap(s) >= srclen);   
    ASSERT_INT (stx_len(s), srclen);   
    ASSERT_STR (s, src);  

    stx_free(s);
    free((void*)src);
}

static void from() 
{
    u_from (0);
    u_from (1);
    u_from (SMALL_MAX/2);
    u_from (SMALL_MAX-1);
    u_from (SMALL_MAX);
    u_from (SMALL_MAX+1);
    u_from (1024);
    u_from (1024*1024); 
    u_from (32*1024*1024); 
}

static void u_from_len (const char* src, size_t srclen, const char* expstr)
{
    stx_t s = stx_from_len(src,srclen);

    if (srclen) assert (stx_cap(s) >= srclen);
    ASSERT_INT (stx_len(s), srclen);   
    ASSERT_STR (s, expstr);      

    stx_free(s);
}

void from_len() 
{
    // binary
    {
        char v[] = {'a','a','\0','b'};
        const size_t len = sizeof(v);
        stx_t s = stx_from_len(v, len);

        assert (stx_cap(s) >= len);
        ASSERT_INT (stx_len(s), len);
        assert (!memcmp(s, v, len));
        
        stx_free(s);
    }

    u_from_len ("", 0, ""); 
    u_from_len ("", 1, ""); 
    u_from_len (foo, 0, ""); 
    u_from_len (foo, foolen-1, "fo");     
    u_from_len (foo, foolen, foo); 
    u_from_len (foo, foolen+1, foo);
    u_from_len (w256, 64, w64); 
    u_from_len (w256, 256, w256); 
    u_from_len (w256, 257, w256); 
    u_from_len (w1024, 64, w64); 
    u_from_len (w1024, 1024, w1024); 
    u_from_len (w1024, 1025, w1024); 
}

//==============================================================================

static void u_dup (const char* src)
{
    stx_t s = stx_from(src);
    stx_t d = stx_dup(s);

    ASSERT_INT (stx_len(d), stx_len(s));   
    ASSERT_STR (s, src);          

    stx_free(s); 
    stx_free(d);
}

static void dup() 
{
    u_dup("");
    u_dup(foo);
    u_dup(w256);
    u_dup(w1024);
}

//==============================================================================

static void u_resize_new (size_t cap, size_t newcap)
{
    stx_t s = stx_new(cap);
    stx_resize(&s, newcap);

    assert (stx_cap(s) >= newcap);   
    ASSERT_INT (stx_len(s), 0);   
    ASSERT_STR (s, "");          
    
    stx_free(s);
}

static void u_resize_from (
    const char* src, size_t newcap, size_t explen, const char* expstr)
{
    stx_t s = stx_from(src);
    stx_resize(&s, newcap);

    assert (stx_cap(s) >= newcap);   
    ASSERT_INT (stx_len(s), explen);   
    ASSERT_STR (s, expstr);          

    stx_free(s);
}

void resize()
{
    u_resize_new (0, 0);
    u_resize_new (0, SMALL_MAX);
    u_resize_new (0, SMALL_MAX-1);
    u_resize_new (0, SMALL_MAX+1);

    u_resize_new (CAP, 0);
    u_resize_new (CAP, CAP);
    u_resize_new (CAP, SMALL_MAX);
    u_resize_new (CAP, SMALL_MAX+1);

    u_resize_new (SMALL_MAX, 0);
    u_resize_new (SMALL_MAX, SMALL_MAX-1);
    u_resize_new (SMALL_MAX, SMALL_MAX);
    u_resize_new (SMALL_MAX, SMALL_MAX+1);

    u_resize_new (SMALL_MAX+1,0);
    u_resize_new (SMALL_MAX+1,SMALL_MAX);
    u_resize_new (SMALL_MAX+1,SMALL_MAX+2);

    u_resize_from ("", 0,   0, "");
    u_resize_from ("", CAP,   0, "");
    u_resize_from ("", SMALL_MAX-1,   0, "");
    u_resize_from ("", SMALL_MAX,   0, "");
    u_resize_from ("", SMALL_MAX+1,   0, "");

    u_resize_from (foo, 0,  0,"");
    u_resize_from (foo, 2, 2, "fo");
    u_resize_from (foo, SMALL_MAX, foolen, foo);
    u_resize_from (foo, SMALL_MAX+1, foolen, foo);

    u_resize_from (w256, 0,  0,"");
    u_resize_from (w256, 64, 64, w64);
    u_resize_from (w256, 256+1, 256, w256);

    u_resize_from (w1024, 0,  0,"");
    u_resize_from (w1024, 64, 64, w64);
    u_resize_from (w1024, 1024+1, 1024, w1024);
}

//==============================================================================

void append()
{
    #define INIT(cap, src, srclen, exprc, expstr) { \
        stx_t s = stx_new(cap); \
        int rc = stx_append (&s, src, srclen); \
        ASSERT_INT (rc, (int)exprc);   \
        ASSERT_STR (s, expstr);        \
        stx_free(s);                                        \
    }

    // TYPE1 -> TYPE1    
    INIT (0,        foo, foolen, foolen, foo);
    INIT (foolen-1, foo, foolen, foolen, foo);
    INIT (foolen  , foo, foolen, foolen, foo);
    INIT (foolen+1, foo, foolen, foolen, foo);

    // TYPE1 -> TYPE4
    INIT (0,        w1024, 1024, 1024, w1024);
    INIT (foolen,   w1024, 1024, 1024, w1024);

    // TYPE4 -> TYPE4
    INIT (1024,   w1024, 1024, 1024, w1024);
    INIT (1024,   w4096, 4096, 4096, w4096);

    // TYPE4 -> TYPE1
    INIT (1024,   foo, foolen, foolen, foo);

    // cut
    INIT (0, foo, foolen-1, foolen-1, "fo");
    INIT (foolen-1, foo, foolen-1, foolen-1, "fo");
    INIT (foolen  , foo, foolen-1, foolen-1, "fo");
    INIT (foolen+1, foo, foolen-1, foolen-1, "fo");

    #undef INIT
}


void append_strict()
{
    #define INIT(cap, src, len,   exprc, explen, expstr) { \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_strict (s, src, len);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        assert_props (s, cap, explen, expstr); \
        stx_free(s);                                        \
    }

    // null: user concern
    // INIT (foolen,    NULL, foolen,    0,     0, "");    
    // no room
    INIT (0, foo, foolen, -foolen, 0, "");
    INIT (0, w256, 256, -256, 0, "");
    // under
    INIT (foolen-1, foo, foolen,    -foolen, 0, "");
    INIT (256-1, w256, 256,    -256, 0, "");
    // exact
    INIT (foolen, foo, foolen,    foolen, foolen, foo);
    INIT (256, w256, 256,    256, 256, w256);
    // over
    INIT (foolen+1, foo, foolen,    foolen, foolen, foo);
    INIT (256+1, w256, 256,    256, 256, w256);
    // cut
    INIT (foolen, foo, foolen-1,  foolen-1,   foolen-1,   "fo");
    INIT (256, w256, 64,    64, 64, W64);
    #undef INIT

    #define MORE(cap, src, len, exprc, explen, expstr) { \
        stx_t s = stx_new(cap);  \
        stx_append_strict (s, foo, foolen);       \
        int rc = stx_append_strict (s, src, len);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        assert_props (s, cap, explen, expstr); \
        stx_free(s); \
    }
    
    // todo w256
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
    #define INIT(cap, fmt, src, exprc, explen, expstr) { \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_fmt (&s, fmt, src);                 \
        ASSERT_INT (rc, (int)exprc); \
        ASSERT_INT (stx_len(s), (int)explen); \
        ASSERT_STR (s, expstr); \
        stx_free(s);                                        \
    }

    INIT (foolen,   "%s", "",   0,        0,      "");
    INIT (foolen,   "%s", foo,  foolen,   foolen, foo);
    INIT (foolen+1, "%s", foo,  foolen,  foolen, foo);
    INIT (foolen-1, "%s", foo,  foolen,   foolen, foo);
    #undef INIT

    #define MORE(cap, fmt, src, exprc, explen, expstr) { \
        stx_t s = stx_new(cap);                            \
        stx_append_strict (s, foo, foolen);                 \
        int rc = stx_append_fmt (&s, fmt, src);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        ASSERT_INT (stx_len(s), (int)explen); \
        ASSERT_STR (s, expstr); \
        stx_free(s);                                        \
    }

    MORE (foobarlen, "%s", bar, foobarlen, foobarlen, foobar);
    #undef MORE
}


void append_fmt_strict()
{
    #define INIT(cap, fmt, src, exprc, explen, expstr) { \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_fmt_strict (s, fmt, src);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        assert_props (s, cap, explen, expstr); \
        stx_free(s);                                        \
    }

    INIT (foolen,   "%s", foo,  foolen,  foolen, foo);
    INIT (foolen+1, "%s", foo,  foolen,  foolen, foo);
    INIT (foolen-1, "%s", foo,  -foolen, 0,      "");
    #undef INIT

    #define MIX(cap, fmt, src1, src2, exprc, explen, expstr) {  \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_fmt_strict (s, fmt, src1, src2);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        assert_props (s, cap, explen, expstr); \
        stx_free(s);                                        \
    }

    MIX (foobarlen,   "%s%s", foo, bar, foobarlen, foobarlen, foobar);
    MIX (foobarlen+1, "%s%s", foo, bar, foobarlen, foobarlen, foobar);
    MIX (foobarlen-1, "%s%s", foo, bar, -foobarlen, 0, "");
    #undef MIX

    #define MORE(cap, fmt, src, exprc, explen, expstr) {   \
        stx_t s = stx_new(cap);                            \
        stx_append_strict (s, foo, foolen);                 \
        int rc = stx_append_fmt_strict (s, fmt, src);                 \
        ASSERT_INT (rc, (int)exprc);                                \
        assert_props (s, cap, explen, expstr); \
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

#define LIST_LOCAL_MAX (STX_LOCAL_MEM/sizeof(stx_t))
#define LIST_POOL_MAX (STX_LIST_POOL_MEM/sizeof(stx_t))

void u_split(splitter fun)
{
    // split_cust (fun, foo,                 "",     0,  (char*[]){""});
    split_cust (fun, FOO,                SEP,    1,  (char*[]){FOO});
    split_cust (fun, FOO SEP,            SEP,    2,  (char*[]){FOO,""});
    split_cust (fun, FOO SEP BAR,        SEP,    2,  (char*[]){FOO,BAR});
    split_cust (fun, FOO SEP BAR SEP,    SEP,    3,  (char*[]){FOO,BAR,""});
    split_cust (fun, FOO SEP SEP BAR,    SEP,    3,  (char*[]){FOO,"",BAR});
    split_cust (fun, "baaaad",    "aa",    3,  (char*[]){"b","","d"});

    split_pat (fun, FOO, SEP, LIST_LOCAL_MAX-1);
    split_pat (fun, FOO, SEP, LIST_LOCAL_MAX);
    split_pat (fun, FOO, SEP, LIST_LOCAL_MAX+1);

    split_pat (fun, FOO, SEP, LIST_POOL_MAX-1);
    split_pat (fun, FOO, SEP, LIST_POOL_MAX);
    split_pat (fun, FOO, SEP, LIST_POOL_MAX+1);

    // split_pat (fun, FOO, SEP, 20000000);
}

void split() {u_split(stx_split_len);}

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
    stx_append_strict (a, w256, 256); //nop
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
    run (new);
    run (from);
    run (from_len);
    run (dup);
    run (join);
    run (split);
    run (append);
    run (append_strict);
    run (append_fmt);
    run (append_fmt_strict);
    run (resize);
    run (reset);
    run (adjust);
    run (trim);
    run (equal);
    run (story);

    printf ("unit tests OK\n");
    return 0;
}