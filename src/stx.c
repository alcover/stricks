/*
Stricks v0.2.0
Copyright (C) 2021 - Francois Alcover <francois@alcover.fr>
NO WARRANTY EXPRESSED OR IMPLIED.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#include "stx.h"
#include "log.h"
#include "util.c"

typedef unsigned char uchar;

typedef struct Head1 {   
    uint8_t     cap;  
    uint8_t     len; 
} Head1;

typedef struct Head4 {   
    uint32_t    cap;  
    uint32_t    len; 
} Head4;

typedef struct HeadS {   
    size_t    cap;  
    size_t    len; 
} HeadS;

typedef struct Attr {   
    uchar   cookie; 
    uchar   flags;
    uchar    data[]; 
} Attr;

typedef enum Type {
    TYPE1 = (int)log2(sizeof(Head1)), 
    TYPE4 = (int)log2(sizeof(Head4))
} Type;

#define ISPOW2(n) (n==2||n==4||n==8||n==16||n==32)
static_assert (ISPOW2(sizeof(Head1)), "bad Head1");
static_assert (ISPOW2(sizeof(Head4)), "bad Head4");
static_assert ((1<<TYPE1) == sizeof(Head1), "bad TYPE1");
static_assert ((1<<TYPE4) == sizeof(Head4), "bad TYPE4");

#define MAGIC 170 // 0xaa 10101010
#define TYPE_BITS 2
#define TYPE_MASK ((1<<TYPE_BITS)-1)

#define COOKIE(s) (((uchar*)(s))[-2])
#define FLAGS(s)  (((uchar*)(s))[-1])
#define TYPE(s) (FLAGS(s) & TYPE_MASK)
#define HEADSZ(type) (1<<type)
#define MEMSZ(type,cap) (HEADSZ(type) + sizeof(Attr) + cap + 1)
#define CHECK(s) ((s) && COOKIE(s) == MAGIC)
#define HEAD(s) ((char*)(s) - sizeof(Attr) - HEADSZ(TYPE(s)))
#define ATTR(head,type) ((Attr*)((char*)(head) + HEADSZ(type)))
#define DATA(head,type) ((char*)head + HEADSZ(type) + sizeof(Attr))

#define SETPROP(head, type, prop, val) \
switch(type) { \
    case TYPE1: ((Head1*)head)->prop = val; break; \
    case TYPE4: ((Head4*)head)->prop = val; break; \
} 

#define GETPROP(head, type, prop) \
((type == TYPE4) ? ((Head4*)head)->prop : ((Head1*)head)->prop)

//==== PRIVATE ===============================================================


static bool 
resize (stx_t *ps, const size_t newcap)
{    
    stx_t s = *ps;

    if (!CHECK(s)) return false;

    void* head = HEAD(s);
    Type type = TYPE(s);
    const size_t cap = GETPROP(head, type, cap);
    const size_t len = GETPROP(head, type, len);

    if (newcap == cap) return true;
    
    const Type newtype = (newcap >= 256) ? TYPE4 : TYPE1;
    const bool sametype = (newtype == type);
   
    void* newhead;

    if (sametype)
        newhead = realloc (head, MEMSZ(type, newcap));
    else {
        newhead = STX_MALLOC (MEMSZ(newtype, newcap));
    }

    if (!newhead) {
        ERR ("stx_resize: realloc failed\n");
        return false;
    }
    
    stx_t news = DATA(newhead, newtype);
    
    if (!sametype) {
        memcpy(news, s, len+1); //?
        // reput len
        SETPROP(newhead, newtype, len, len);
        // reput cookie
        COOKIE(news) = MAGIC;
        // update flags
        FLAGS(news) = (FLAGS(news) & ~TYPE_MASK) | newtype;
    }
    
    // truncated
    if (newcap < len) {
        #ifdef STX_WARNINGS
            LOG("stx_resize: truncated");
        #endif
        SETPROP(newhead, newtype, len, newcap);
    }

    // update cap
    SETPROP(newhead, newtype, cap, newcap); 
    // update cap sentinel
    news[newcap] = 0;
    
    *ps = news;
    return true;
}

//==== PUBLIC ================================================================

stx_t 
stx_new (const size_t cap)
{
    const Type type = (cap >= 256) ? TYPE4 : TYPE1;

    void* head = STX_MALLOC (MEMSZ(type, cap));
    if (!head) return NULL;

    SETPROP(head, type, cap, cap);
    SETPROP(head, type, len, 0);

    Attr* attr = ATTR(head, type);
    attr->cookie = MAGIC;
    attr->flags = type;
    attr->data[0] = 0; 
    attr->data[cap] = 0; 
    
    return (char*)(attr->data);
}

const stx_t
stx_from (const char* src)
{
    const size_t len = strlen(src);
    const stx_t ret = stx_new(len);

    stx_append_count (ret, src, len);

    return ret;
}

void 
stx_free (const stx_t s)
{
    if (!CHECK(s)) return;

    char* head = HEAD(s);
    
    switch(TYPE(s)) {
        case TYPE4: bzero(head, sizeof(Head4) + sizeof(Attr)); break;
        case TYPE1: bzero(head, sizeof(Head1) + sizeof(Attr)); break;
    }

    STX_FREE(head);
}

#define ACCESS(s, prop) \
if (!CHECK(s)) return 0; \
void* head = HEAD(s); \
switch(TYPE(s)){ \
    case TYPE1: return ((Head1*)head)->prop; \
    case TYPE4: return ((Head4*)head)->prop; \
    default: return 0; \
}

size_t 
stx_cap (const stx_t s) {ACCESS(s,cap);}

size_t 
stx_len (const stx_t s) {ACCESS(s,len);}


// int 
// append (void* dst, const char* src, const size_t n, bool alloc, bool strict) 
// {
//     stx_t s = NULL;
//     stx_t *ps = NULL;
    
//     if (alloc) s = (stx_t) dst;


//     if (!CHECK(s)||!src) return STX_FAIL;

//     void* head = HEAD(s);
//     Type type = TYPE(s);
//     size_t cap, len;
    
//     getdims(head, type, &cap, &len);

//     const size_t inc = n ? strnlen(src,n) : strlen(src);
//     const size_t totlen = len + inc;

//     if (totlen > cap) {  
//         // Would truncate, return needed capacity
//         return -totlen;
//     }

//     char* end = s + len;
//     memcpy (end, src, inc);
//     end[inc] = 0;
//     SETPROP(head, type, len, totlen);

//     return inc;        
// }

int 
stx_append_count (stx_t s, const char* src, const size_t n) 
{
    if (!CHECK(s)||!src) return STX_FAIL;

    const void* head = HEAD(s);
    const Type type = TYPE(s);
    const size_t cap = GETPROP(head, type, cap);
    const size_t len = GETPROP(head, type, len);

    const size_t inc = n ? strnlen(src,n) : strlen(src);
    const size_t totlen = len + inc;

    if (totlen > cap) {  
        // Would truncate, return needed capacity
        return -totlen;
    }

    char* end = s + len;
    memcpy (end, src, inc);
    end[inc] = 0;
    SETPROP(head, type, len, totlen);

    return inc;        
}


size_t 
stx_append_count_alloc (stx_t *ps, const char *src, const size_t n)
{
    stx_t s = *ps;

    if (!CHECK(s)||!src) return STX_FAIL;
    
    const void* head = HEAD(s);
    const Type type = TYPE(s);
    const size_t cap = GETPROP(head, type, cap);
    const size_t len = GETPROP(head, type, len);
    
    const size_t inc = n ? strnlen(src,n) : strlen(src);
    const size_t totlen = len + inc;

    if (totlen > cap) {
        if (!resize(ps, totlen*2)) {
            ERR("resize failed");
            return STX_FAIL;
        }
        s = *ps;
        head = HEAD(s);
    }
    
    char* end = s + len;
    memcpy (end, src, inc);
    end[inc] = 0;
    SETPROP(head, type, len, totlen);        

    return inc;
}


bool
stx_resize (stx_t *ps, const size_t newcap)
{
    return resize(ps, newcap);
}


void 
stx_show (const stx_t s)
{
    if (!CHECK(s)) return;

    void* head = HEAD(s);
    int type = TYPE(s);

    #define SHOW_FMT "cap:%zu len:%zu cookie:%d flags:%d data:'%s'\n"
    #define SHOW_ARGS (size_t)(h->cap), (size_t)(h->len), (uchar)s[-2], (uchar)s[-1], s

    switch(type){
        case TYPE4: {
            Head4* h = (Head4*)head;
            printf (SHOW_FMT, SHOW_ARGS);
            break;
        }
        case TYPE1: {
            Head1* h = (Head1*)head;
            printf (SHOW_FMT, SHOW_ARGS);
            break;
        }
        default: ERR("stx_show: unknown type\n");
    }

    fflush(stdout);
}