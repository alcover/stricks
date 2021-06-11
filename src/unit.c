#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#define STX_WARNINGS 1
#include "stx.h"
#define ENABLE_LOG 1
#include "log.h"
#include "util.c"
//==============================================================================

#define assert_cmp(a,b) assert(!strcmp((a),(b))) 

#define ASSERT_PROPS(s, cap, len, data) \
ASSERT_INT (stx_cap(s), cap); \
ASSERT_INT (stx_len(s), len); \
ASSERT_STR ((s), data)


#define ASSERT_INT(a, b) { \
    if ((a)!=(b)) { \
        fprintf(stderr, "%d: %s:%d != %s:%d\n", \
            __LINE__, #a, (int)a, #b, (int)b); \
        exit(1); \
    } \
}

#define ASSERT_STR(a, b) { \
    if (strcmp(a, b)) { \
        fprintf(stderr, "%d: %s:'%s' != %s:'%s'\n", \
            __LINE__, #a, a, #b, b); \
        exit(1); \
    } \
}

//==============================================================================

#define CAP 100  

const char* foo = "foo";
const size_t foolen = 3;
const char* bar = "bar";
const size_t barlen = 3;
const char* foobar = "foobar";
const size_t foobarlen = 6;

#define X16(s) #s #s #s #s #s #s #s #s #s #s #s #s #s #s #s #s 
#define BIG X16(aaaabbbbccccdddd)
#define biglen 256
// static_assert(strlen(BIG)==biglen, "biglen != 256");
const char* big = BIG;

void new() 
{ 
    stx_t s;

    s = stx_new(0);
    ASSERT_PROPS (s, 0, 0, ""); 
    
    s = stx_new(CAP);
    ASSERT_PROPS (s, CAP, 0, ""); 
    
    stx_free(s);
}

void from() 
{
    stx_t s;
    
    s = stx_from (NULL);
    ASSERT_PROPS (s, 0, 0, ""); 

    s = stx_from ("");
    ASSERT_PROPS (s, 0, 0, ""); 

    s = stx_from(foo);
    ASSERT_PROPS (s, foolen, foolen, foo); 

    stx_free(s);
}

void from_len() 
{
    stx_t s;

    s = stx_from_len (foo, foolen);
    ASSERT_PROPS (s, foolen, foolen, foo); 

    s = stx_from_len (foo, foolen-1);
    ASSERT_PROPS (s, foolen-1, foolen-1, "fo");     

    /* fails if from_len uses blind memcpy */
    // s = stx_from_len (foo, foolen+1);
    // ASSERT_PROPS (s, foolen+1, foolen, foo); 

    s = stx_from_len (foo, 0);
    ASSERT_PROPS (s, 0, 0, ""); 
    
    s = stx_from_len (NULL, 0);
    ASSERT_PROPS (s, 0, 0, ""); 

    s = stx_from_len (NULL, CAP);
    ASSERT_PROPS (s, 0, 0, ""); 

    s = stx_from_len ("", 0);
    ASSERT_PROPS (s, 0, 0, ""); 
    
    /* fails if from_len uses blind memcpy */
    // s = stx_from_len ("", CAP);
    // ASSERT_PROPS (s, CAP, 0, "");

    stx_free(s);
}

void dup() 
{
    stx_t s = stx_new(foolen+1);
    stx_append_strict (s, foo, foolen);
    stx_t dup = stx_dup(s);
    ASSERT_PROPS (dup, foolen, foolen, foo);
    stx_free(s);
}


void free_() 
{
    stx_t s = stx_new(3); //stx_from(foo);
    stx_free(s);

    // double free
    stx_free(s);
    
    // use after free
    assert (!stx_cap(s));
    assert (!stx_len(s));
    assert (!stx_spc(s));
    assert (!stx_resize (&s,10));
    assert (!stx_dup(s));
    assert (0 == stx_append (&s, foo, foolen));
    assert (0 == stx_append_strict (s, foo, foolen));
    assert (0 == stx_append_fmt (s, "%s", foo));
}


void resize()
{
    stx_t s;

    s = stx_new(CAP);
    stx_resize(&s, 0);
    ASSERT_PROPS (s, 0, 0, ""); 

    s = stx_new(CAP);
    stx_resize(&s, 1);
    ASSERT_PROPS (s, 1, 0, ""); 

    s = stx_new(CAP);
    stx_resize(&s, CAP);
    ASSERT_PROPS (s, CAP, 0, ""); 

    s = stx_new(CAP);
    stx_resize(&s, CAP+1);
    ASSERT_PROPS (s, CAP+1, 0, ""); 

    s = stx_new(CAP);
    stx_append_strict(s,foo, foolen);
    stx_resize(&s, 2);
    ASSERT_PROPS (s, 2, 2, "fo"); 

    s = stx_new(CAP);
    stx_append_strict(s,foo, foolen);
    stx_resize(&s, foolen+1);
    ASSERT_PROPS (s, foolen+1, foolen, foo); 


    stx_free(s);
}

void reset()
{
    stx_t s = stx_new(CAP);
    stx_append_strict (s, foo, foolen);
    stx_reset(s);
    ASSERT_PROPS (s, CAP, 0, ""); 
    stx_free(s);
}

void adjust()
{
    stx_t s = stx_new(CAP);
    stx_append_strict (s, foo, foolen);
    ((char*)s)[foolen-1] = 0;
    stx_adjust(s);
    ASSERT_PROPS (s, CAP, foolen-1, "fo"); 
    stx_free(s);
}

void check()
{
    stx_t s = stx_new(3);
    assert(stx_check(s));

    stx_append_strict (s, foo, foolen);
    assert(stx_check(s));

    ((char*)s)[-2] = 0; 
    assert(!stx_check(s));

    s = stx_new(3);
    stx_free(s);
    assert(!stx_check(s));
}

void equal()
{
    stx_t a = stx_new(3);
    stx_t b = stx_new(4);
    assert(stx_equal(a,b));

    stx_append_strict (a, foo, foolen);
    stx_append_strict (b, foo, foolen);
    assert(stx_equal(a,b));

    stx_append_strict (b, "o", 1);
    assert(!stx_equal(a,b));
}

void trim()
{
    char* str;
    size_t len;
    stx_t s;

    str = " foo "; len = strlen(str);
    s = stx_new(CAP);
    stx_append_strict (s, str, len);
    stx_trim(s);
    ASSERT_PROPS(s, CAP, foolen, foo);

    str = "foo "; len = strlen(str);
    s = stx_new(CAP);
    stx_append_strict (s, str, len);
    stx_trim(s);
    ASSERT_PROPS(s, CAP, foolen, foo);
}

//==============================================================================

void append()
{
    #define INIT(cap, src, len, exprc, expdata) { \
        stx_t s = stx_new(cap); \
        int rc = stx_append (&s, src, len); \
        assert (rc == (int)exprc);                               \
        assert_cmp (s, expdata);                            \
        stx_free(s);                                        \
    }

    // big dest, no realloc
    INIT (biglen+1, big, biglen, biglen, big);
    // big src
    INIT (foolen  , big, biglen, biglen, big);

    // exact count
    INIT (foolen  , foo, foolen, foolen, foo);
    INIT (foolen+1, foo, foolen, foolen, foo);
    INIT (foolen-1, foo, foolen, foolen, foo);
    // under count
    INIT (foolen  , foo, foolen-1, foolen-1, "fo");
    INIT (foolen+1, foo, foolen-1, foolen-1, "fo");
    INIT (foolen-1, foo, foolen-1, foolen-1, "fo");
    // over count
    // INIT (foolen  , foo, foolen+1, foolen+1, foo);
    // INIT (foolen+1, foo, foolen+1, foolen, foo);
    // INIT (foolen-1, foo, foolen+1, foolen, foo);

    #undef INIT
}

void append_strict()
{
    #define INIT(cap, src, len, exprc, explen, expdata)    \
    {                                                       \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_strict (s, src, len);                 \
        ASSERT_INT (rc, (int)exprc);                               \
        ASSERT_PROPS (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    // exact count
    INIT (foolen,    foo, foolen,    foolen,     foolen, foo);
    INIT (foolen+1,  foo, foolen,    foolen,     foolen, foo);
    INIT (foolen-1,  foo, foolen,    -foolen,    0,      "");
    // under count
    INIT (foolen,    foo, foolen-1, foolen-1,    foolen-1,   "fo");
    INIT (foolen+1,  foo, foolen-1, foolen-1,    foolen-1,   "fo");
    INIT (foolen-1,  foo, foolen-1, foolen-1,    foolen-1,   "fo");
    // over count
    // INIT (foolen,    foo, foolen+1,  foolen,      foolen, foo);
    // INIT (foolen+1,  foo, foolen+1,  foolen,      foolen, foo);
    // INIT (foolen-1,  foo, foolen+1,  -foolen,    0,      "");

    #undef INIT

    #define MORE(cap, src1, src2, count, exprc, explen, expdata) \
    { \
        stx_t s = stx_new(cap);  \
        stx_append_strict (s, src1, strlen(src1));       \
        int rc = stx_append_strict (s, src2, count);                 \
        if (rc != (int)exprc) {printf("rc:%d exprc:%d\n", rc, (int)exprc); fflush(stdout);} \
        assert (rc == (int)exprc);                               \
        ASSERT_PROPS (s, cap, explen, expdata); \
        stx_free(s); \
    }
    
    // exact count
    MORE (foobarlen+1, foo, bar, barlen, barlen, foobarlen, foobar);
    MORE (foobarlen  , foo, bar, barlen, barlen, foobarlen, foobar);
    MORE (foobarlen-1, foo, bar, barlen, -foobarlen, foolen, foo);
    // under count
    MORE (foobarlen+1, foo, bar, barlen-1, barlen-1, foobarlen-1, "fooba");
    MORE (foobarlen  , foo, bar, barlen-1, barlen-1, foobarlen-1, "fooba");
    MORE (foobarlen-1, foo, bar, barlen-1, barlen-1, foobarlen-1, "fooba");
    // over count
    // MORE (foobarlen+1, foo, bar, barlen+1, barlen,   foobarlen,  foobar);
    // MORE (foobarlen  , foo, bar, barlen+1, barlen,   foobarlen,  foobar);
    // MORE (foobarlen-1, foo, bar, barlen+1, -foobarlen, foolen,   foo);

    #undef MORE
}


void append_fmt()
{
    #define INIT(cap, fmt, src, exprc, explen, expdata)    \
    {                                                       \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_fmt (s, fmt, src);                 \
        assert (rc == (int)exprc);                               \
        ASSERT_PROPS (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    #define INIT2(cap, fmt, src1, src2, exprc, explen, expdata)    \
    {                                                       \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_fmt (s, fmt, src1, src2);                 \
        assert (rc == (int)exprc);                               \
        ASSERT_PROPS (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    #define MORE(cap, fmt, src1, src2, exprc, explen, expdata)    \
    {                                                       \
        stx_t s = stx_new(cap);                            \
        stx_append_fmt (s, fmt, src1);                 \
        int rc = stx_append_fmt (s, fmt, src2);                 \
        assert (rc == (int)exprc);                               \
        ASSERT_PROPS (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    INIT (foolen,   "%s", foo,  foolen,  foolen, foo);
    INIT (foolen+1, "%s", foo,  foolen,  foolen, foo);
    INIT (foolen-1, "%s", foo,  -foolen, 0,      "");

    INIT2 (foobarlen,   "%s%s", foo, bar, foobarlen, foobarlen, foobar);
    INIT2 (foobarlen+1, "%s%s", foo, bar, foobarlen, foobarlen, foobar);
    INIT2 (foobarlen-1, "%s%s", foo, bar, -foobarlen, 0, "");

    MORE (foobarlen*2, "%s", foo, bar, barlen, foobarlen, foobar);
}

//==============================================================================

void story()
{
    stx_t a = stx_new(foolen);
    stx_append_strict(a,big,biglen); //nop
    stx_append_strict(a,foo,foolen); //a==foo

    stx_t b = stx_from(bar);
    stx_t c = stx_dup(b); //c==bar
    size_t clen = stx_len(c);

    stx_append(&a, c, clen); //a==foobar

    stx_reset(b);
    stx_resize(&b,foobarlen);
    stx_append_strict(b,"", 0);
    stx_append_fmt(b, "%s%s", foo, c); //b==foobar

    assert_cmp(a,b);
    assert(stx_equal(a,b));
}

//==============================================================================

#define FOO "foo"
#define BAR "bar"
#define SEP "|"

typedef stx_t*(*splitter)(const char*, size_t, const char*, size_t, size_t*);

void split_cust(
splitter fun, const char* str, const char* sep, size_t expc, char* exps[])
{
    size_t cnt = 0;
    stx_t* list = fun(str, strlen(str), sep, strlen(sep), &cnt);
    stx_t* l = list;
    stx_t s;
    size_t i=0;

    ASSERT_INT(cnt,expc);

    while ((s = *l++)) {
        const char* exp = exps[i++];
        const size_t explen = strlen(exp);
        ASSERT_PROPS (s, explen, explen, exp);
    }

    // stx_list_free(list);
}


// (foo, |, 3) => "foo|foo|foo|" => 4 : foo,foo,foo,""
void split_pat (splitter fun, const char* word, const char* sep, size_t n) 
{ 
    const char* pat = str_cat(word,sep); //LOGVS(pat);
    const char* txt = str_repeat(pat,n); //LOGVS(txt);
    const size_t wordlen = strlen(word);
    size_t cnt = 0;

    stx_t* list = fun(txt, strlen(txt), sep, strlen(sep), &cnt);

    ASSERT_INT(cnt,n+1);
    
    for (size_t i = 0; i < cnt-1; ++i) {
        stx_t s = list[i];
        ASSERT_PROPS (s, wordlen, wordlen, word);
    }
    ASSERT_STR(list[cnt-1], "");

    // free((char*)pat);
    // free((char*)txt);
    // stx_list_free(list);
}

void split_unit(splitter fun)
{
    split_cust (fun, "",                 SEP,    1,  (char*[]){""});
    split_cust (fun, FOO,                SEP,    1,  (char*[]){FOO});
    split_cust (fun, FOO SEP,            SEP,    2,  (char*[]){FOO,""});
    split_cust (fun, FOO SEP BAR,        SEP,    2,  (char*[]){FOO,BAR});
    split_cust (fun, FOO SEP BAR SEP,    SEP,    3,  (char*[]){FOO,BAR,""});
    split_cust (fun, FOO SEP SEP BAR,    SEP,    3,  (char*[]){FOO,"",BAR});

    split_pat (fun, FOO, SEP, 6);
    split_pat (fun, FOO, SEP, STX_STACK_MAX-1);
    split_pat (fun, FOO, SEP, STX_STACK_MAX);
    split_pat (fun, FOO, SEP, STX_STACK_MAX+1);
    split_pat (fun, FOO, SEP, STX_POOL_MAX-1);
    split_pat (fun, FOO, SEP, STX_POOL_MAX);
    split_pat (fun, FOO, SEP, STX_POOL_MAX+1);

    split_pat (fun, FOO, SEP, 20000000);
}

void split() {split_unit(stx_split_len);}
// void split_fast() {split_unit(stx_split_fast);}

//==============================================================================

#define run(name) { \
    printf("%s ", #name); fflush(stdout); \
    name(); \
    LOG("OK"); \
}

int main()
{
    run(new);
    run(from);
    run(from_len);
    run(append);
    run(append_strict);
    run(append_fmt);
    run(dup);
    run(reset);
    run(adjust);
    run(resize);
    run(check);
    run(free_);
    run(equal);
    run(trim);
    // run(str_count_str_);
    run(story);
    
    run(split);
    // run(split_fast);

    printf ("unit tests OK\n");
    return 0;
}