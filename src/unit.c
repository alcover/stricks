/*
    Unit tests of public methods
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#define STX_SHORT_NAMES
#include "stx.h"
#include "log.h"

#define assert_cmp(a,b) assert(!strcmp((a),(b))) 

int main()
{
    const char* foo = "foo";
    const size_t foolen = strlen(foo);
    const char* bar = "bar";
    const size_t barlen = strlen(bar);

    char foobar[6] = {0};
    strcat(foobar, foo);
    strcat(foobar, bar);
    const size_t foobarlen = strlen(foobar);


    #define ASSERT_PROPS(s, cap, len, data)    \
    {                                                   \
        assert (stx_cap(s) == cap); \
        if (stx_len(s)!=len) { \
            printf ("exp:%zu len:%zu\n", (size_t)len, (size_t)stx_len(s)); fflush(stdout);} \
        assert (stx_len(s) == len);   \
        assert (!strcmp(s, data));         \
    }    

    /**** stx_new ********************************************************/

    {
        const int cap = 100;
        stx_t s = stx_new(cap);
        ASSERT_PROPS (s, cap, 0, ""); 
        stx_free(s);
    }

    /**** stx_from ********************************************************/
 
    {
        stx_t s = stx_from(foo);
        ASSERT_PROPS (s, foolen, foolen, foo); 
        stx_free(s);
    }

    /**** stx_free ********************************************************/

    {
        stx_t s = stx_from(foo);
        stx_free(s);

        // double free
        stx_free(s);

        // use after free
        assert (!stx_cap(s));
        assert (!stx_len(s));
        assert (!stx_spc(s));
        assert (!stx_resize (&s,10));
        assert (!stx_dup(s));
        assert (STX_FAIL == stx_append (s, bar));
        assert (STX_FAIL == stx_append_alloc (&s, bar));
        assert (STX_FAIL == stx_append_format (s, "%s", bar));
        assert (STX_FAIL == stx_append_count (s, bar, 3));
        assert (STX_FAIL == stx_append_count_alloc (&s, bar, 3));
    }

    /**** stx_reset ********************************************************/

    {
        const int cap = 100;
        stx_t s = stx_new(cap);
        stx_append (s, foo);
        stx_reset(s);
        ASSERT_PROPS (s, cap, 0, ""); 
        stx_free(s);
    }

    /**** stx_check ********************************************************/

    {
        stx_t s = stx_new(foolen);
        assert (stx_check(s));
        stx_append (s, foo);
        assert (stx_check(s));
        stx_append_count_alloc (&s, bar, 0);
        assert (stx_check(s));
        stx_reset(s);
        assert (stx_check(s));
        stx_free(s);
        assert (!stx_check(s));
    }


    /**** stx_dup ***************************************************************/

    {
        stx_t s = stx_new(foolen+1);
        stx_append (s, foo);
        stx_t dup = stx_dup(s);
        ASSERT_PROPS (dup, foolen, foolen, foo);
        stx_free(s);
    }

    {
        stx_t s = stx_from(foo);
        stx_t dup = stx_dup(s);
        ASSERT_PROPS (dup, foolen, foolen, foo);
        stx_free(s);
    }

    {
        stx_t s = stx_new(foolen);
        stx_t dup = stx_dup(s);
        ASSERT_PROPS (dup, 0, 0, "");
        stx_free(s);
    }

    /**** stx_append_count *******************************************************/

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

    /**** stx_append *******************************************************/

    {
        const size_t cap = foobarlen;
        int rc;
        stx_t s = stx_new(cap);

        rc = stx_append (s, foo);
        assert (rc == foolen);
        ASSERT_PROPS (s, cap, foolen, foo);

        rc = stx_append (s, bar);
        assert (rc == barlen);
        ASSERT_PROPS (s, cap, foobarlen, foobar);

        rc = stx_append (s, bar);
        // LOGVI(rc);
        assert (rc == -(foobarlen + barlen));
        ASSERT_PROPS (s, cap, foobarlen, foobar);

        stx_free(s);
    }

    /**** stx_append_count_alloc *******************************************************/

    #define APPENDA(s, src, count, exprc, expdata)    \
    {                                                       \
        int rc = stx_append_count_alloc (&s, src, count);                 \
        assert (rc == exprc);                               \
        assert_cmp (s, expdata);                            \
    }

    #define APPENDA_INIT(cap, src, count, exprc, expdata)    \
    {                                                       \
        stx_t s = stx_new(cap);                            \
        APPENDA (s, src, count, exprc, expdata);            \
        stx_free(s);                                        \
    }

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

    /**** stx_append_format *******************************************************/

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

    /**** stx_resize ******************************************************/


    #define APPEND(s, src, exprc, expdata)    \
    {                                                       \
        int rc = stx_append(s, src);                 \
        assert (rc == exprc);                               \
        assert_cmp (s, expdata);                            \
    }    


    #define RESIZE(s, newcap, exprc, explen, expdata) \
    { \
        const size_t len = stx_len(s); \
        bool rc = stx_resize(&s, newcap); \
        assert (rc == exprc);      \
        ASSERT_PROPS (s, newcap, explen, expdata); \
    }

    {
        stx_t s;


        // neg
        s = stx_new(foolen);         
        assert (false == stx_resize(&s, -3));
        stx_free(s);

        // init
        s = stx_new(foolen);         
        RESIZE (s, foolen+1, true, 0, "");
        RESIZE (s, foolen-1, true, 0, "");
        stx_free(s);

        // fit
        s = stx_from(foo);
        RESIZE (s, foolen+1, true, foolen, foo);
        RESIZE (s, foolen-1, true, foolen-1, "fo");
        stx_free(s);

        // story
        s = stx_new(foolen+1);
        stx_append (s, foo);
        const size_t needsz = - stx_append(s, bar);
        assert (needsz == foobarlen);
        RESIZE (s, needsz, true, foolen, foo);
        APPEND (s, bar, barlen, foobar);

        stx_free(s);
    }

    /**** story ***************************************************************/

    {
        const size_t cap = foolen+1;
        stx_t a = stx_new(cap);
        stx_t b = stx_from(bar);
        stx_append(a, foo);
        stx_append(a, b);
        stx_resize(&a, foobarlen);
        stx_append(a, b);
        ASSERT_PROPS (a, foobarlen, foobarlen, foobar);
    }

    /**************************************************************************/

    printf ("unit tests OK\n");
    return 0;
}