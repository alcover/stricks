// Unit tests of public methods

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#include "stx.h"
#include "log.h"

#define assert_cmp(a,b) assert(!strcmp((a),(b))) 

#define ASSERT_PROPS(s, cap, len, data) \
assert (stx_cap(s) == cap); \
assert (stx_len(s) == len); \
assert (!strcmp(s, data))  

#define CAP 100  

const char* foo = "foo";
const char* bar = "bar";
const char* foobar = "foobar";
const size_t foolen = 3;
const size_t barlen = 3;
const size_t foobarlen = 6;

#define X16(s) #s #s #s #s #s #s #s #s #s #s #s #s #s #s #s #s 
#define BIG X16(aaaabbbbccccdddd)
#define biglen 256
static_assert(strlen(BIG)==biglen, "biglen != 256");
const char* big = BIG;

void new() 
{ 
    stx_t s = stx_new(CAP);
    ASSERT_PROPS (s, CAP, 0, ""); 
    stx_free(s);
}

void from() 
{
    stx_t s = stx_from(foo, 0);
    ASSERT_PROPS (s, foolen, foolen, foo); 
    s = stx_from(foo, foolen-1);
    ASSERT_PROPS (s, foolen-1, foolen-1, "fo");     
    s = stx_from(foo, foolen+1);
    ASSERT_PROPS (s, foolen, foolen, foo); 

    stx_free(s);
}

void append()
{
    #define APPEND_INIT(cap, src, count, exprc, explen, expdata)    \
    {                                                       \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_count (s, src, count);                 \
        assert (rc == exprc);                               \
        ASSERT_PROPS (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    // auto count
    APPEND_INIT (foolen,    foo, 0,         foolen,     foolen, foo);
    APPEND_INIT (foolen+1,  foo, 0,         foolen,     foolen, foo);
    APPEND_INIT (foolen-1,  foo, 0,         -foolen,    0,      "");

    // exact count
    APPEND_INIT (foolen,    foo, foolen,    foolen,     foolen, foo);
    APPEND_INIT (foolen+1,  foo, foolen,    foolen,     foolen, foo);
    APPEND_INIT (foolen-1,  foo, foolen,    -foolen,    0,      "");

    // under count
    APPEND_INIT (foolen,    foo, foolen-1, foolen-1,    foolen-1,   "fo");
    APPEND_INIT (foolen+1,  foo, foolen-1, foolen-1,    foolen-1,   "fo");
    APPEND_INIT (foolen-1,  foo, foolen-1, foolen-1,    foolen-1,   "fo");

    // over count
    APPEND_INIT (foolen,    foo, foolen+1,  foolen,      foolen, foo);
    APPEND_INIT (foolen+1,  foo, foolen+1,  foolen,      foolen, foo);
    APPEND_INIT (foolen-1,  foo, foolen+1,  -foolen,    0,      "");


    #define APPEND_MORE(cap, src1, src2, count, exprc, explen, expdata) \
    { \
        stx_t s = stx_new(cap);  \
        stx_append (s, src1);       \
        int rc = stx_append_count (s, src2, count);                 \
        if (rc != exprc) {printf("rc:%d exprc:%d\n", rc, (int)exprc); fflush(stdout);} \
        assert (rc == exprc);                               \
        ASSERT_PROPS (s, cap, explen, expdata); \
        stx_free(s); \
    }
    
    // auto count
    APPEND_MORE (foobarlen+1, foo, bar, 0, barlen, foobarlen, foobar);
    APPEND_MORE (foobarlen  , foo, bar, 0, barlen, foobarlen, foobar);
    APPEND_MORE (foobarlen-1, foo, bar, 0, -foobarlen, foolen, foo);

    // exact count
    APPEND_MORE (foobarlen+1, foo, bar, barlen, barlen, foobarlen, foobar);
    APPEND_MORE (foobarlen  , foo, bar, barlen, barlen, foobarlen, foobar);
    APPEND_MORE (foobarlen-1, foo, bar, barlen, -foobarlen, foolen, foo);

    // under count
    APPEND_MORE (foobarlen+1, foo, bar, barlen-1, barlen-1, foobarlen-1, "fooba");
    APPEND_MORE (foobarlen  , foo, bar, barlen-1, barlen-1, foobarlen-1, "fooba");
    APPEND_MORE (foobarlen-1, foo, bar, barlen-1, barlen-1, foobarlen-1, "fooba");
    
    // over count
    APPEND_MORE (foobarlen+1, foo, bar, barlen+1, barlen,   foobarlen,  foobar);
    APPEND_MORE (foobarlen  , foo, bar, barlen+1, barlen,   foobarlen,  foobar);
    APPEND_MORE (foobarlen-1, foo, bar, barlen+1, -foobarlen, foolen,   foo);
}

void append_alloc()
{
    #define APPENDA_INIT(cap, src, count, exprc, expdata) { \
        stx_t s = stx_new(cap); \
        int rc = stx_append_count_alloc (&s, src, count); \
        assert (rc == exprc);                               \
        assert_cmp (s, expdata);                            \
        stx_free(s);                                        \
    }

    // big dest, no realloc
    APPENDA_INIT (biglen+1, big, 0, biglen, big);
    // big src
    APPENDA_INIT (foolen  , big, 0, biglen, big);

    // auto count
    APPENDA_INIT (foolen  , foo, 0, foolen, foo);
    APPENDA_INIT (foolen+1, foo, 0, foolen, foo);
    APPENDA_INIT (foolen-1, foo, 0, foolen, foo);
    // exact count
    APPENDA_INIT (foolen  , foo, foolen, foolen, foo);
    APPENDA_INIT (foolen+1, foo, foolen, foolen, foo);
    APPENDA_INIT (foolen-1, foo, foolen, foolen, foo);

    APPENDA_INIT (foolen  , foo, foolen-1, foolen-1, "fo");
    APPENDA_INIT (foolen+1, foo, foolen-1, foolen-1, "fo");
    APPENDA_INIT (foolen-1, foo, foolen-1, foolen-1, "fo");

    APPENDA_INIT (foolen  , foo, foolen+1, foolen, foo);
    APPENDA_INIT (foolen+1, foo, foolen+1, foolen, foo);
    APPENDA_INIT (foolen-1, foo, foolen+1, foolen, foo);
}

void append_fmt()
{
    #define APPENDF_INIT(cap, fmt, src, exprc, explen, expdata)    \
    {                                                       \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_format (s, fmt, src);                 \
        assert (rc == exprc);                               \
        ASSERT_PROPS (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    #define APPENDF_INIT2(cap, fmt, src1, src2, exprc, explen, expdata)    \
    {                                                       \
        stx_t s = stx_new(cap);                            \
        int rc = stx_append_format (s, fmt, src1, src2);                 \
        assert (rc == exprc);                               \
        ASSERT_PROPS (s, cap, explen, expdata); \
        stx_free(s);                                        \
    }

    APPENDF_INIT (foolen,   "%s", foo,  foolen,  foolen, foo);
    APPENDF_INIT (foolen+1, "%s", foo,  foolen,  foolen, foo);
    APPENDF_INIT (foolen-1, "%s", foo,  -foolen, 0,      "");

    APPENDF_INIT2 (foobarlen,   "%s%s", foo, bar, foobarlen,  foobarlen,    foobar);
    APPENDF_INIT2 (foobarlen+1, "%s%s", foo, bar, foobarlen,  foobarlen,    foobar);
    APPENDF_INIT2 (foobarlen-1, "%s%s", foo, bar, -foobarlen, 0,            "");
}

void dup() 
{
    stx_t s = stx_new(foolen+1);
    stx_append (s, foo);
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
    assert (0 == stx_append (s, bar));
    assert (0 == stx_append_alloc (&s, bar));
    assert (0 == stx_append_format (s, "%s", bar));
    assert (0 == stx_append_count (s, bar, 3));
    assert (0 == stx_append_count_alloc (&s, bar, 3));
}

void reset()
{
    stx_t s = stx_new(CAP);
    stx_append (s, foo);
    stx_reset(s);
    ASSERT_PROPS (s, CAP, 0, ""); 
    stx_free(s);
}

void check()
{
    stx_t s = stx_new(3);
    assert(stx_check(s));

    stx_append (s, foo);
    assert(stx_check(s));

    s[-2] = 0; 
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

    stx_append (a, foo);
    stx_append (b, foo);
    assert(stx_equal(a,b));

    stx_append (b, "o");
    assert(!stx_equal(a,b));
}

void trim()
{
    stx_t s = stx_new(CAP);
    stx_append (s, " foo ");
    stx_trim(s);
    ASSERT_PROPS(s, CAP, foolen, foo);
}

// todo cases
void split()
{
    stx_t s = stx_from("foo, foo", 0);
    unsigned cnt = 0;
    stx_t* list = stx_split(s, ", ", &cnt);
    // LOGVI(cnt);
    assert(cnt==2);

    for (int i = 0; i < cnt; ++i) {
        stx_t elt = list[i];
        // printf("tok %s\n", elt); fflush(stdout);
        ASSERT_PROPS(elt, foolen, foolen, foo);
    }
}

//=======================================================================================
#define U(name) name(); printf("passed %s\n", #name)

int main()
{
    U(new);
    U(from);
    U(append);
    U(append_alloc);
    U(append_fmt);
    U(dup);
    U(reset);
    U(check);
    U(free_);
    U(equal);
    U(trim);
    U(split);

    printf ("unit tests OK\n");
    return 0;
}