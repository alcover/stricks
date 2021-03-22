// todo : free() all instances

// Unit tests of public methods

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#define STX_WARNINGS 0
#include "stx.h"
#include "log.h"
// #include "util.h"
#include "util.c"
//======================================================================================

#define assert_cmp(a,b) assert(!strcmp((a),(b))) 

#define ASSERT_PROPS(s, cap, len, data) \
ASSERT_INT (stx_cap(s), cap); \
ASSERT_INT (stx_len(s), len); \
ASSERT_STR ((s), data)


#define ASSERT_INT(a, b) { \
    if (a!=b) { \
        fprintf(stderr, "%d: %s:%d != %s:%d\n", \
            __LINE__, #a, (int)a, #b, (int)b); \
        exit(1); \
    } \
}

#define ASSERT_STR(stra, strb) { \
    if (strcmp(stra, strb)) { \
        fprintf(stderr, "%d: %s:'%s' != %s:'%s'\n", \
            __LINE__, #stra, stra, #strb, strb); \
        exit(1); \
    } \
}

//======================================================================================

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
    ASSERT_PROPS (s, STX_MIN_CAP, 0, ""); 
    
    s = stx_new(CAP);
    ASSERT_PROPS (s, CAP, 0, ""); 
    
    stx_free(s);
}

void from() 
{
    stx_t s;
    
    s = stx_from (NULL);
    ASSERT_PROPS (s, STX_MIN_CAP, 0, ""); 

    s = stx_from ("");
    ASSERT_PROPS (s, STX_MIN_CAP, 0, ""); 

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

    s = stx_from_len (foo, foolen+1);
    ASSERT_PROPS (s, foolen+1, foolen, foo); 

    s = stx_from_len (foo, 0);
    ASSERT_PROPS (s, STX_MIN_CAP, 0, ""); 
    
    s = stx_from_len (NULL, 0);
    ASSERT_PROPS (s, STX_MIN_CAP, 0, ""); 

    s = stx_from_len (NULL, CAP);
    ASSERT_PROPS (s, CAP, 0, ""); 

    s = stx_from_len ("", 0);
    ASSERT_PROPS (s, STX_MIN_CAP, 0, ""); 
    
    s = stx_from_len ("", CAP); //stx_show(s);
    ASSERT_PROPS (s, CAP, 0, ""); //!

    stx_free(s);
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


void resize()
{
    stx_t s;

    s = stx_new(CAP);
    stx_resize(&s, 0);
    ASSERT_PROPS (s, CAP, 0, ""); 

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
    stx_append(s,foo);
    stx_resize(&s, 2);
    ASSERT_PROPS (s, 2, 2, "fo"); 

    s = stx_new(CAP);
    stx_append(s,foo);
    stx_resize(&s, foolen+1);
    ASSERT_PROPS (s, foolen+1, foolen, foo); 


    stx_free(s);
}

void reset()
{
    stx_t s = stx_new(CAP);
    stx_append (s, foo);
    stx_reset(s);
    ASSERT_PROPS (s, CAP, 0, ""); 
    stx_free(s);
}

void update()
{
    stx_t s = stx_new(CAP);
    stx_append (s, foo);
    ((char*)s)[foolen-1] = 0;
    stx_update(s);
    ASSERT_PROPS (s, CAP, foolen-1, "fo"); 
    stx_free(s);
}

void check()
{
    stx_t s = stx_new(3);
    assert(stx_check(s));

    stx_append (s, foo);
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

    s = stx_new(CAP);
    stx_append (s, "foo ");
    stx_trim(s);
    ASSERT_PROPS(s, CAP, foolen, foo);
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
    // under count
    APPENDA_INIT (foolen  , foo, foolen-1, foolen-1, "fo");
    APPENDA_INIT (foolen+1, foo, foolen-1, foolen-1, "fo");
    APPENDA_INIT (foolen-1, foo, foolen-1, foolen-1, "fo");
    // over count
    APPENDA_INIT (foolen  , foo, foolen+1, foolen, foo);
    APPENDA_INIT (foolen+1, foo, foolen+1, foolen, foo);
    APPENDA_INIT (foolen-1, foo, foolen+1, foolen, foo);
}



// void append_fmt_more(src1, src2)
// {
//     stx_t s = stx_new(cap);                            
//     int rc = stx_append_format (s, fmt, src);                 
//     assert (rc == exprc);                               
//     ASSERT_PROPS (s, cap, explen, expdata); 
//     stx_free(s);                                        
// }

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

    #define APPENDF_MORE(cap, fmt, src1, src2, exprc, explen, expdata)    \
    {                                                       \
        stx_t s = stx_new(cap);                            \
        stx_append_format (s, fmt, src1);                 \
        int rc = stx_append_format (s, fmt, src2);                 \
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

    APPENDF_MORE (foobarlen*2, "%s", foo, bar, barlen,  foobarlen,    foobar);

    {                                                       
        stx_t page = stx_new(128);
        int votes = 1;
        char user[20] = "Paul";
        char text[100] = "First post!";
        char* fmt = "■ %s (%d points) %s";
        char check[100];
        snprintf (check, 100, fmt, user, votes, text);
        // LOGVS(check);
        size_t len = strlen(check);
        // LOGI(len);
        int rc = stx_append_format (page, fmt, user, votes, text);
        // LOGVI(rc);                                     
        assert (rc == len);
        ASSERT_PROPS (page, 128, len, "■ Paul (1 points) First post!");
        // stx_free(s);                     
    }

}





void split_unit (const char* str, const char* sep, int expcnt, char* expparts[])
{
    unsigned cnt = 0;
    stx_t* list = stx_split(str, strlen(str), sep, &cnt);

    ASSERT_INT(cnt,expcnt);

    for (int i = 0; i < cnt; ++i) {
        const stx_t elt = list[i];
        const char* exp = expparts[i];
        const size_t explen = strlen(exp);
        // ASSERT_PROPS (elt, STX_MIN_CAP, 1, explen);
        ASSERT_INT (stx_cap(elt), max(STX_MIN_CAP,explen));
        ASSERT_INT (stx_len(elt), explen);
        ASSERT_STR (elt, exp);
    }
}


void split()
{
    // split_unit ("", NULL, 1, (char*[]){""});
    // split_unit ("", "", 1, (char*[]){""});
    // split_unit ("", ",", 1, (char*[]){""});

    split_unit ("ab", NULL, 1, (char*[]){"ab"});
    split_unit ("ab", "", 1, (char*[]){"ab"});
    split_unit ("a,b", ",", 2, (char*[]){"a","b"});

    split_unit ("a,,c", ",", 3, (char*[]){"a","","c"});
}


void story()
{
    stx_t a = stx_new(foolen);
    stx_append(a,big); //nop
    stx_append(a,foo);
    stx_append_alloc(&a,bar); //foobar

    stx_t b = stx_from(foo);
    stx_t c = stx_dup(b);
    stx_resize(&c,foobarlen);
    stx_append_format(c,"%s",bar); //foobar

    // append nothing
    stx_reset(a);
    stx_append(a,"");
    stx_append(c,a);

    assert_cmp(a,c);
    assert(stx_equal(a,c));
}

//=======================================================================================

#define BOLD  "\033[1m"
#define RESET "\033[0m"

#define run(name) { \
    printf(#name " "); \
    name(); \
    printf( BOLD "passed.\n" RESET); \
    fflush(stdout); \
}

int main()
{
    run(new);
    run(from);
    run(from_len);
    run(append);
    run(append_alloc);
    run(append_fmt);
    run(dup);
    run(reset);
    run(update);
    run(resize);
    run(check);
    run(free_);
    run(equal);
    run(trim);
    run(split);

    printf ("unit tests OK\n");
    return 0;
}