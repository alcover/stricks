/*
Stricks v0.4.2
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
} Prop1;

typedef struct {   
    uint32_t cap;  
    uint32_t len; 
} Prop4;

typedef struct {   
    uint8_t canary; 
    uint8_t flags;
    char data[]; 
} Attr;

static_assert (sizeof(Attr) == 2, "bad Attr");

// comptime math : gcc ok, clang error ?
typedef enum {
    TYPE1 = 1, // (int)log2(sizeof(Prop1)), 
    TYPE4 = 3  // (int)log2(sizeof(Prop4))
} Type;

static_assert (sizeof(Prop1)==(1<<TYPE1), "bad TYPE1");
static_assert (sizeof(Prop4)==(1<<TYPE4), "bad TYPE4");

#define MAGIC 0xaa
#define TYPE_BITS 2
#define TYPE_MASK ((1<<TYPE_BITS)-1)
#define SMALL_MAX 255 // = max 8bit strlen(data)
#define DATAOFF offsetof(Attr,data)

#define FLAGS(s) (((uint8_t*)(s))[-1]) // faster than safer attr->flags ?
#define TYPE(s) (FLAGS(s) & TYPE_MASK)
#define PROPSZ(type) (1<<type)
#define HEAD(s) ((char*)(s) - DATAOFF - PROPSZ(TYPE(s)))
#define HEADT(s,type) ((char*)(s) - DATAOFF - PROPSZ(type))

#define ATTR(s) ((Attr*)((char*)(s) - DATAOFF))
#define HATTR(head,type) ((Attr*)((char*)(head) + PROPSZ(type)))

#define CANARY(s) ATTR(s)->canary

#define TOTSZ(type,cap) (PROPSZ(type) + DATAOFF + cap + 1)
#define CHECK(s) ((s) && (CANARY(s) == MAGIC))
#define DATA(head,type) ((char*)head + PROPSZ(type) + DATAOFF)

#define HGETPROP(head, type, prop) \
((type == TYPE4) ? ((Prop4*)head)->prop : ((Prop1*)head)->prop)
#define HGETCAP(head, type) HGETPROP(head, type, cap)
#define HGETLEN(head, type) HGETPROP(head, type, len)

#define HSETPROP(head, type, prop, val) \
switch(type) { \
    case TYPE1: ((Prop1*)head)->prop = val; break; \
    case TYPE4: ((Prop4*)head)->prop = val; break; \
    default: ERR("Bad head type"); exit(1); \
} 
#define HSETCAP(head, type, val) HSETPROP(head, type, cap, val)
#define HSETLEN(head, type, val) HSETPROP(head, type, len, val)
#define HSETPROPS(head, type, cap, len) \
switch(type) { \
    case TYPE1: *((Prop1*)head) = (Prop1){cap,len}; break; \
    case TYPE4: *((Prop4*)head) = (Prop4){cap,len}; break; \
    default: ERR("Bad head type"); exit(1); \
} 

static inline size_t 
HGETSPC (const void* head, Type type){
    switch(type) { 
        case TYPE4: return ((Prop4*)head)->cap - ((Prop4*)head)->len;
        case TYPE1: return ((Prop1*)head)->cap - ((Prop1*)head)->len;
    }  
}

#define LEN_TYPE(len) ((len <= SMALL_MAX) ? TYPE1 : TYPE4)

//==== PRIVATE =================================================================

stx_t list_pool[STX_POOL_MAX] = {NULL};

static inline size_t
getlen (stx_t s)
{
    const Type type = TYPE(s);
    void* head = HEADT(s, type);
    return HGETLEN(head, type);
}

static inline void
setlen (stx_t s, size_t len)
{
    const Type type = TYPE(s);
    void* head = HEADT(s, type);
    HSETLEN(head, type, len);
}

static inline bool 
resize (stx_t *ps, size_t newcap)
{    
    stx_t s = *ps;

    const Type type = TYPE(s);
    const void* head = HEADT(s, type);
    const size_t cap = HGETCAP(head, type);
    const size_t len = HGETLEN(head, type);

    if (newcap == cap) return true;
    // newcap < cap : optim realloc only if < 1/2 ?

    const Type newtype = LEN_TYPE(newcap);
    const bool sametype = (newtype == type);
    const size_t newsize = TOTSZ(newtype, newcap);
    
    void* newhead = sametype ? STX_REALLOC((void*)head, newsize)
                             : STX_MALLOC(newsize);

    if (!newhead) {
        ERR ("stx_resize: realloc failed\n");
        return false;
    }
    
    char* newdata = DATA(newhead, newtype);
    size_t newlen = min(len,newcap);
    
    if (!sametype) {
        
        // copy attributes + data
        memcpy (newdata-DATAOFF, s-DATAOFF, DATAOFF+newlen); //?
        newdata[newlen] = 0; //nec?
        
        // update type
        FLAGS(newdata) = (FLAGS(newdata) & ~TYPE_MASK) | newtype;

        free((void*)head);
    }
    
    HSETCAP(newhead, newtype, newcap); 
    // update cap sentinel
    newdata[newcap] = 0;

    // if (newlen != len) 
        HSETLEN(newhead, newtype, newlen);
    
    *ps = newdata;

    return true;
}


static long long 
append (stx_t* dst, const void* src, size_t srclen, bool strict) 
{
    stx_t s = *dst;
    
    if (!CHECK(s)) return 0;

    const Type type = TYPE(s);
    void* head = HEADT(s, type);
    const size_t cap = HGETCAP(head, type);
    const size_t len = HGETLEN(head, type);
    const size_t totlen = len + srclen;

    if (totlen > cap) {  
        // Would truncate, return needed capacity
        if (strict) return -totlen;

        if (!resize(&s, totlen*2)) {
            ERR("resize failed");
            return 0;
        }
        *dst = s;
    }

    char* end = (char*)s + len;
    memcpy (end, src, srclen);
    end[srclen] = 0;
    setlen(s, totlen);

    return totlen;        
}


static long long 
append_fmt (stx_t* dst, bool strict, const char* fmt, va_list args) 
{
    if (!dst) return 0;
    
    stx_t s = *dst;
    if (!CHECK(s)) return 0;

    va_list argscpy;
    va_copy(argscpy, args);
    
    errno = 0;
    const int inlen = vsnprintf(NULL, 0, fmt, args);

    if (inlen < 0) {
        perror("vsnprintf");
        return 0;
    }

    Type type = TYPE(s);
    void* head = HEADT(s, type);
    const size_t cap = HGETCAP(head, type);
    const size_t len = HGETLEN(head, type);
    const size_t totlen = len+inlen;
     
    // Truncation
    if (totlen > cap) {

        if (strict) {
            STX_WARN("stx: append_fmt strict: would truncate\n");
            return -totlen;            
        }
         
        if (!resize(dst, totlen)) {
            ERR("resize failed");
            return 0;
        }
        s = *dst;
        type = LEN_TYPE(totlen);
        head = HEADT(s, type);
    } 

    vsprintf((char*)s+len, fmt, argscpy);
    va_end(argscpy);

    // Update length
    HSETLEN(head, type, totlen);

    return totlen;           
}


// copy only up to current length
static stx_t
dup (stx_t src)
{
    const Type type = TYPE(src);
    const void* head = HEADT(src, type);
    const size_t len = HGETLEN(head, type);
    const size_t cpysz = TOTSZ(type,len);
    void* new_head = malloc(cpysz);

    if (!new_head) return NULL;

    memcpy(new_head, head, cpysz);
    HSETCAP(new_head, type, len);
    stx_t ret = DATA(new_head, type);
    ((char*)ret)[len] = 0;

    return ret;
}



static inline stx_t 
new (size_t cap)
{
    const Type type = LEN_TYPE(cap);
    void* head = STX_MALLOC(TOTSZ(type, cap));
    if (!head) return NULL;

    HSETPROPS(head, type, cap, 0);

    Attr* attr = HATTR(head,type);
    char* data = attr->data;

    attr->canary = MAGIC;
    attr->flags = type;
    data[0] = 0; 
    data[cap] = 0; 
    
    return data;
}

static inline stx_t 
from (const char* src, size_t srclen)
{
    const Type type = LEN_TYPE(srclen);
    void* head = STX_MALLOC(TOTSZ(type, srclen));
    if (!head) return NULL;

    HSETPROPS(head, type, srclen, srclen);

    Attr* attr = HATTR(head,type);
    char* data = attr->data; //DATA(head,type);

    attr->canary = MAGIC;
    attr->flags = type;
    memcpy (data, src, srclen); //optim argscpy srclen+1 ?
    data[srclen] = 0; 

    return data;
}

static inline stx_t 
from_to (const char* src, size_t srclen, char* head)
{
    const Type type = LEN_TYPE(srclen);
    Attr* attr = HATTR(head,type);
    char* data = attr->data;

    HSETPROPS(head, type, srclen, srclen);
    attr->canary = MAGIC;
    attr->flags = type;
    memcpy (data, src, srclen); //optim copy srclen+1 ?
    data[srclen] = 0; 

    return data;
}

static void
dbg (stx_t s)
{
    #define FMT "cap:%zu len:%zu canary:%x flags:%x data:\"%s\"\n"
    #define ARGS (size_t)(h->cap), (size_t)(h->len), CANARY(s), FLAGS(s), s

    if (!CHECK(s)) ERR("stx_dbg: invalid");

    const void* head = HEAD(s);
    const Type type = TYPE(s);

    switch(type){
        case TYPE4: {
            Prop4* h = (Prop4*)head;
            printf(FMT, ARGS);
            break;
        }
        case TYPE1: {
            Prop1* h = (Prop1*)head;
            printf(FMT, ARGS);
            break;
        }
        default: ERR("stx_dbg: unknown type\n");
    }

    #undef FMT
    #undef ARGS
    fflush(stdout);
}

//==== PUBLIC ==================================================================

stx_t 
stx_new (size_t cap)
{
    return new(cap);
}

stx_t
stx_from (const char* src)
{
    return src ? from(src, strlen(src)) : new(0);
}

// strncpy_s ?
stx_t
stx_from_len (const void* src, size_t srclen)
{
    return src ? from(src, srclen) : new(0);
}

stx_t
stx_dup (stx_t src)
{
    return CHECK(src) ? dup(src) : NULL;
}

void 
stx_reset (stx_t s)
{
    if (!CHECK(s)) return;
    setlen(s,0);
    *((char*)s) = 0;
} 

void 
stx_free (stx_t s)
{
    if (!CHECK(s)) {
        STX_WARN("stx_free: invalid header\n");
        return;
    }

    const Type type = TYPE(s);
    void* head = HEADT(s, type);
    // safe crossing structs ?
    memset (head, 0, PROPSZ(type) + DATAOFF + 1);
    STX_FREE(head);
}


size_t 
stx_cap (stx_t s) 
{
    if (!CHECK(s)) return 0;  
    return HGETCAP(HEAD(s), TYPE(s));
}

size_t 
stx_len (stx_t s) 
{
    if (!CHECK(s)) return 0;
    return getlen(s);
}

size_t 
stx_spc (stx_t s)
{
    if (!CHECK(s)) return 0;
    return HGETSPC(HEAD(s), TYPE(s));
}

size_t 
stx_append (stx_t* dst, const void* src, size_t srclen)
{
    return append(dst, src, srclen, false);        
}

long long 
stx_append_strict (stx_t dst, const void* src, size_t srclen) 
{
    return append(&dst, src, srclen, true);       
}



size_t 
stx_append_fmt (stx_t* dst, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    long long ret = append_fmt(dst, false, fmt, args);
    va_end(args);

    return ret;
} 

long long 
stx_append_fmt_strict (stx_t dst, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    long long ret = append_fmt(&dst, true, fmt, args);
    va_end(args);

    return ret;
} 

bool
stx_resize (stx_t *ps, size_t newcap)
{
    if (!ps||!CHECK(*ps)) return false;
    return resize(ps, newcap);
}

// nb: memcmp(,,0) == 0
bool 
stx_equal (stx_t a, stx_t b) 
{
    if (!CHECK(a)||!CHECK(b)) return false;

    const size_t lena = getlen(a);
    const size_t lenb = getlen(b);

    return (lena == lenb) && !memcmp(a, b, lena);
}

bool stx_check (stx_t s) {return CHECK(s);}
void stx_dbg (stx_t s) {dbg(s);}


// todo new fit type ?
void 
stx_trim (stx_t s)
{
    if (!CHECK(s)) return;
    
    const char* front = s;
    while (isspace(*front)) ++front;

    const char* end = s + getlen(s);
    while (end > front && isspace(*(end-1))) --end;
    
    const size_t newlen = end-front;
    
    if (front > s) {
        memmove((void*)s, front, newlen);
    }
    
    ((char*)s)[newlen] = 0;
    setlen(s,newlen);
}


void
stx_adjust (stx_t s)
{
    if (!CHECK(s)) return;
    setlen(s,strlen(s));
}


stx_t*
stx_split_len (const char* src, size_t srclen, const char* sep, size_t seplen, size_t* outcnt)
{
    size_t cnt = 0; 
    stx_t* ret = NULL;
    
    if (!src||!seplen) goto fin;

    stx_t  list_local[STX_STACK_MAX]; 
    stx_t *list_dyn = NULL;
    stx_t *list_reloc = NULL;
    stx_t *list = list_local;
    size_t listmax = STX_STACK_MAX;

    const char *beg = src;
    const char *end = src;

    while (end = strstr(end, sep)) {

        if (cnt >= listmax-2) { // -2 : last part + sentinel

            if (list == list_local) {

                list_reloc = list_pool;
                listmax = STX_POOL_MAX;

            } else {

                listmax *= 2;
                const size_t newsz = listmax * sizeof(stx_t);

                if (list == list_pool) {

                    list_dyn = STX_MALLOC (newsz); 
                    if (!list_dyn) {cnt = 0; goto fin;}
                    list_reloc = list_dyn;
                
                } else { 

                    list = STX_REALLOC (list, newsz); 
                    if (!list) {cnt = 0; goto fin;}
                }
            }
        
            if (list_reloc) {
                memcpy (list_reloc, list, cnt * sizeof(stx_t));
                list = list_reloc;
                list_reloc = NULL;
            }
        }

        list[cnt++] = from(beg, end-beg);

        end += seplen;
        beg = end;
    };
    
    // part after last sep
    list[cnt++] = from(beg, src+srclen-beg);

    if (list == list_dyn) {
        ret = list;  
    } else {
        ret = STX_MALLOC((cnt+1) * sizeof(stx_t)); // +1: sentinel
        if (!ret) {cnt = 0; goto fin;}
        memcpy (ret, list, cnt * sizeof(stx_t));
    }

    ret[cnt] = NULL; // sentinel

    fin:
    *outcnt = cnt;
    return ret;
}


stx_t*
stx_split (const char* src, const char* sep, size_t* outcnt)
{
    const size_t srclen = sep ? strlen(src) : 0;
    const size_t seplen = sep ? strlen(sep) : 0;
    return stx_split_len(src, srclen, sep, seplen, outcnt);
}

void
stx_list_free (const stx_t *list)
{
    const stx_t *l = list;
    stx_t s;

    while ((s = *l++)) stx_free(s); //bug unit
    free((void*)list); //not for pool !
}