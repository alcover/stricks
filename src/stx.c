/*
Stricks v0.2.2
Copyright (C) 2021 - Francois Alcover <francois[@]alcover.fr>
NO WARRANTY EXPRESSED OR IMPLIED.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <ctype.h> // isspace
#include <math.h>
#include <assert.h>
#include <errno.h>

#include "stx.h"
#include "log.h"
#include "util.c"

typedef struct {   
    uint8_t cap;
    uint8_t len; 
} Head1;

typedef struct {   
    uint32_t cap;  
    uint32_t len; 
} Head4;

typedef struct {   
    uint8_t cookie; 
    uint8_t flags;
    char data[]; 
} Attr;

typedef enum {
    TYPE1 = (int)log2(sizeof(Head1)), 
    TYPE4 = (int)log2(sizeof(Head4))
} Type;

static_assert ((1<<TYPE1) == sizeof(Head1), "bad TYPE1");
static_assert ((1<<TYPE4) == sizeof(Head4), "bad TYPE4");

#define MAGIC 0xaa
#define TYPE_BITS 2
#define TYPE_MASK ((1<<TYPE_BITS)-1)

#define COOKIE(s) (((uint8_t*)(s))[-2])
#define FLAGS(s)  (((uint8_t*)(s))[-1])
#define TYPE(s) (FLAGS(s) & TYPE_MASK)
#define TYPESZ(type) (1<<type)
#define MEMSZ(type,cap) (TYPESZ(type) + sizeof(Attr) + cap + 1)
#define CHECK(s) ((s) && COOKIE(s) == MAGIC)
#define HEAD(s) ((char*)(s) - sizeof(Attr) - TYPESZ(TYPE(s)))
#define ATTR(head,type) ((Attr*)((char*)(head) + TYPESZ(type)))
#define DATA(head,type) ((char*)head + TYPESZ(type) + sizeof(Attr))

#define HSETCAP(head, type, val) HSETPROP(head, type, cap, val)
#define HSETLEN(head, type, val) HSETPROP(head, type, len, val)
#define HSETPROP(head, type, prop, val) \
switch(type) { \
    case TYPE1: ((Head1*)head)->prop = val; break; \
    case TYPE4: ((Head4*)head)->prop = val; break; \
} 

#define HGETCAP(head, type) HGETPROP(head, type, cap)
#define HGETLEN(head, type) HGETPROP(head, type, len)
#define HGETPROP(head, type, prop) \
((type == TYPE4) ? ((Head4*)head)->prop : ((Head1*)head)->prop)

#define HGETSPC(head, type) \
((type == TYPE4) ? (((Head4*)head)->cap - ((Head4*)head)->len) \
                 : (((Head1*)head)->cap - ((Head1*)head)->len))

#define SETPROP(s,prop,val) HSETPROP(HEAD(s), TYPE(s), prop, val)
#define GETPROP(s,prop) HGETPROP(HEAD(s), TYPE(s), prop)
#define GETSPC(s) HGETSPC(HEAD(s), TYPE(s))

static intmax_t append(void* dst, const char* src, const size_t n, bool alloc);
static bool resize (stx_t *ps, const size_t newcap);
static intmax_t append_format (stx_t dst, const char* fmt, va_list args);
static stx_t dup (const stx_t s);

//==== PUBLIC =======================================================================

const stx_t 
stx_new (const size_t mincap)
{
    const size_t cap = max(mincap, STX_MIN_CAP);
    const Type type = (cap >= 256) ? TYPE4 : TYPE1;

    void* head = STX_MALLOC(MEMSZ(type, cap));
    if (!head) return NULL;

    HSETCAP(head, type, cap);
    HSETLEN(head, type, 0);

    Attr* attr = ATTR(head, type);
    attr->cookie = MAGIC;
    attr->flags = type;
    attr->data[0] = 0; 
    attr->data[cap] = 0; 
    
    return attr->data;
}

const stx_t
stx_from (const char* src, const size_t n)
{
    if (!src) return stx_new(n);
    
    const size_t len = n ? strnlen(src,n) : strlen(src);
    stx_t ret = stx_new(len);

    memcpy(ret, src, len);
    SETPROP(ret, len, len);

    return ret;
}

const stx_t
stx_dup (const stx_t s)
{
    return CHECK(s) ? dup(s) : NULL;
}

void 
stx_reset (const stx_t s)
{
    if (!CHECK(s)) return;
    SETPROP(s, len, 0);
    *s = 0;
} 

void 
stx_free (const stx_t s)
{
    if (!CHECK(s)) {
        #ifdef STX_WARNINGS
            ERR ("stx_free: invalid header\n");
        #endif
        return;
    }

    void* head = HEAD(s);
    
    switch(TYPE(s)) {
        case TYPE4: bzero(head, sizeof(Head4) + sizeof(Attr)); break;
        case TYPE1: bzero(head, sizeof(Head1) + sizeof(Attr)); break;
    }

    STX_FREE(head);
}


size_t 
stx_cap (const stx_t s) 
{
    if (!CHECK(s)) return 0;  
    return GETPROP(s, cap);  
}

size_t 
stx_len (const stx_t s) 
{
    if (!CHECK(s)) return 0;
    return GETPROP(s, len);
}

size_t 
stx_spc (const stx_t s)
{
    if (!CHECK(s)) return 0;
    return GETSPC(s);
}

intmax_t 
stx_append (stx_t dst, const char* src) 
{
    return append((void*)dst, src, 0, false);       
}

intmax_t 
stx_append_count (stx_t dst, const char* src, const size_t n) 
{
    return append((void*)dst, src, n, false);       
}

size_t 
stx_append_alloc (stx_t* dst, const char* src)
{
    return append((void*)dst, src, 0, true);        
}

size_t 
stx_append_count_alloc (stx_t* dst, const char* src, const size_t n)
{
    return append((void*)dst, src, n, true);        
}

intmax_t 
stx_append_format (const stx_t dst, const char* fmt, ...) 
{
    if (!CHECK(dst)) return 0;

    va_list args;
    va_start(args, fmt);
    intmax_t rc = append_format (dst, fmt, args);            
    va_end(args);

    return rc;
}

bool
stx_resize (stx_t *ps, const size_t newcap)
{
    return resize(ps, newcap);
}

// nb: memcmp(,,0) == 0
bool 
stx_equal (const stx_t a, const stx_t b) 
{
    if (!CHECK(a)||!CHECK(b)) return false;

    const size_t lena = GETPROP((a), len);
    const size_t lenb = GETPROP((b), len);

    return (lena == lenb) && !memcmp(a, b, lena);
}

bool 
stx_check (const stx_t s)
{
    return CHECK(s);
}

void 
stx_show (const stx_t s)
{
    if (!CHECK(s)) return;

    const void* head = HEAD(s);
    const Type type = TYPE(s);

    #define SHOW_FMT "cap:%zu len:%zu cookie:%x flags:%x data:'%s'\n"
    #define SHOW_ARGS (size_t)(h->cap), (size_t)(h->len), ((uint8_t*)s)[-2], ((uint8_t*)s)[-1], s

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


// todo new fit type ?
void 
stx_trim (const stx_t s)
{
    if (!CHECK(s)) return;
    
    const char* front = s;
    while (isspace(*front)) ++front;

    const char* end = s + GETPROP(s,len);
    while (end > front && isspace(*(end-1))) --end;
    
    const size_t newlen = end-front;
    
    if (front > s) {
        memmove(s, front, newlen);
    }
    
    s[newlen] = 0;
    SETPROP(s, len, newlen);
}


typedef struct {
    stx_t* list; 
    unsigned int cnt;
} split_ctx;


static void 
split_callback (const char* tok, const size_t len, void* ctx)
{
    split_ctx* c = ctx;

    if (!c->cnt) {
        ERR("split_callback: smth wrong");
        return; 
    }

    stx_t out = len ? stx_from(tok, len) : stx_from("", 0);

    *(c->list++) = out;
    --c->cnt;
}


// sentinel to allow `while(part++)`
stx_t*
stx_split (const void* s, const char* sep, unsigned int* outcnt)
{
    if (!s) {
        *outcnt = 0;
        return NULL;
    }

    const unsigned int cnt = str_count(s,sep) + 1;
    stx_t* list = STX_MALLOC((cnt+1) * sizeof(*list)); // +1: sentinel

    if (sep) {
        split_ctx ctx = {list,cnt};
        str_split (s, sep, split_callback, &ctx);
    } else {
        *list = stx_from(s,0); // stx_split("foo", NULL) -> "foo"
    }

    *outcnt = cnt;
    list[cnt] = NULL; // sentinel

    return list;
}

//==== PRIVATE =======================================================================

static intmax_t 
append (void* dst, const char* src, const size_t n, bool alloc/*, bool strict*/) 
{
    stx_t s = alloc ? *((stx_t**)(dst)) : dst;
    
    if (!CHECK(s)||!src) return 0;

    const size_t cap = GETPROP(s, cap);
    const size_t len = GETPROP(s, len);
    const size_t inc = n ? strnlen(src,n) : strlen(src);
    const size_t totlen = len + inc;

    if (totlen > cap) {  
        // Would truncate, return needed capacity
        if (!alloc) return -totlen;

        if (!resize(&s, totlen*2)) {
            ERR("resize failed");
            return 0;
        }
        *((stx_t*)(dst)) = s;
    }

    char* end = s + len;
    memcpy (end, src, inc);
    end[inc] = 0;
    SETPROP(s, len, totlen);

    return inc;        
}


static bool 
resize (stx_t *ps, const size_t newcap)
{    
    stx_t s = *ps;

    if (!CHECK(s)) return false;

    const void* head = HEAD(s);
    const Type type = TYPE(s);
    const size_t cap = HGETCAP(head, type);
    const size_t len = HGETLEN(head, type);

    if (newcap == cap) return true;
    
    const Type newtype = (newcap >= 256) ? TYPE4 : TYPE1;
    const bool sametype = (newtype == type);
   
    void* newhead;

    if (sametype)
        newhead = STX_REALLOC ((void*)head, MEMSZ(type, newcap));
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
        HSETLEN(newhead, newtype, len);
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
        HSETLEN(newhead, newtype, newcap);
    }

    // update cap
    HSETCAP(newhead, newtype, newcap); 
    // update cap sentinel
    news[newcap] = 0;
    
    *ps = news;
    return true;
}


static intmax_t 
append_format (stx_t dst, const char* fmt, va_list args)
{
    const void* head = HEAD(dst);
    const Type type = TYPE(dst);
    const size_t len = HGETLEN(head, type);
    const size_t spc = HGETSPC(head, type);

    if (!spc) return 0;

    char* end = dst + len;

    errno = 0;
    const size_t src_len = vsnprintf(end, spc+1, fmt, args);
     
    // Error
    if (src_len < 0) {
        perror("stx_append_format");
        *end = 0; // undo
        return 0;
    }

    // Truncation
    if (src_len > spc) {
        *end = 0; // undo
        return -(len + src_len);
    } 

    // Update length
    HSETLEN(head, type, src_len);

    return src_len;
}


static stx_t
dup (const stx_t s)
{
    const void* head = HEAD(s);
    const Type type = TYPE(s);
    const size_t len = HGETLEN(head, type);
    const size_t sz = MEMSZ(type,len);
    void* new_head = malloc(sz);

    if (!new_head) return NULL;

    memcpy(new_head, head, sz);
    HSETCAP(new_head, type, len);
    stx_t ret = DATA(new_head, type);
    ret[len] = 0;

    return ret;
}