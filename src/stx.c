/*
Stricks v0.4.0
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

#define STX_WARNINGS 1
#include "stx.h"
// #define ENABLE_LOG
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
#define SMALL_CAP 255
#define DATAOFF offsetof(Attr,data)

#define FLAGS(s) (((uint8_t*)(s))[-1]) // faster than safer attr->flags ?
#define TYPE(s) (FLAGS(s) & TYPE_MASK)
#define PROPSZ(type) (1<<type)
#define HEAD(s) ((char*)(s) - DATAOFF - PROPSZ(TYPE(s)))
#define HEADT(s,type) ((char*)(s) - DATAOFF - PROPSZ(type))

#define ATTR(s) ((Attr*)((char*)(s) - DATAOFF))
#define HATTR(head,type) ((Attr*)((char*)(head) + PROPSZ(type)))

#define CANARY(s) ATTR(s)->canary

#define MEMSZ(type,cap) (PROPSZ(type) + DATAOFF + cap + 1)
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


#define LEN_TYPE(len) ((len <= SMALL_CAP) ? TYPE1 : TYPE4)

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
    
    const Type newtype = LEN_TYPE(newcap);
    const bool sametype = (newtype == type);
    const size_t newsize = MEMSZ(newtype, newcap);
   
    void* newhead = sametype ? STX_REALLOC((void*)head, newsize)
                             : STX_MALLOC (newsize);

    if (!newhead) {
        ERR ("stx_resize: realloc failed\n");
        return false;
    }
    
    stx_t newdata = DATA(newhead, newtype);
    
    if (!sametype) {
        memcpy((char*)newdata, s, len+1); //?

        HSETLEN(newhead, newtype, len);
        CANARY(newdata) = MAGIC;
        FLAGS(newdata) = (FLAGS(newdata) & ~TYPE_MASK) | newtype;

        free((void*)head);
    }
    
    // truncated
    if (newcap < len) HSETLEN(newhead, newtype, newcap);

    // update cap
    HSETCAP(newhead, newtype, newcap); 
    
    // update cap sentinel
    ((char*)(newdata))[newcap] = 0;
    
    *ps = newdata;

    return true;
}


// change: no strnlen
static int 
append (void* dst, const char* src, size_t srclen, bool alloc) 
{
    stx_t s = alloc ? *((stx_t**)(dst)) : dst;
    
    if (!CHECK(s)) return 0;

    const Type type = TYPE(s);
    void* head = HEADT(s, type);
    const size_t cap = HGETCAP(head, type);
    const size_t len = HGETLEN(head, type);
    const size_t totlen = len + srclen;

    // s might change !
    if (totlen > cap) {  
        // Would truncate, return needed capacity
        if (!alloc) return -totlen;

        if (!resize(&s, totlen*2)) {
            ERR("resize failed");
            return 0;
        }
        *((stx_t*)(dst)) = s;
    }

    char* end = (char*)s + len;
    memcpy (end, src, srclen);
    end[srclen] = 0;
    setlen(s, totlen);

    return srclen;        
}


static int 
append_format (stx_t dst, const char* fmt, va_list args)
{
    const Type type = TYPE(dst);
    const void* head = HEADT(dst, type);
    const size_t len = HGETLEN(head, type);
    const size_t spc = HGETSPC(head, type);

    if (!spc) return 0;

    char* end = ((char*)dst) + len;

    errno = 0;
    const int fmlen = vsnprintf(end, spc+1, fmt, args);
     
    // Error
    if (fmlen < 0) {
        perror("stx_append_format");
        *end = 0; // undo
        return 0;
    }

    // Truncation
    if (fmlen > (int)spc) {
        #if STX_WARNINGS > 0
            ERR ("append_format: truncation\n");
        #endif
        *end = 0; // undo
        return -(len + fmlen); 
    } 

    // Update length
    HSETLEN(head, type, len + fmlen);

    return fmlen;
}


static stx_t
dup (stx_t src)
{
    const Type type = TYPE(src);
    const void* head = HEADT(src, type);
    const size_t len = HGETLEN(head, type);
    const size_t sz = MEMSZ(type,len);
    void* new_head = malloc(sz);

    if (!new_head) return NULL;

    memcpy(new_head, head, sz);
    HSETCAP(new_head, type, len);
    stx_t ret = DATA(new_head, type);
    ((char*)ret)[len] = 0;

    return ret;
}



static inline stx_t 
new (size_t cap)
{
    const Type type = LEN_TYPE(cap);
    void* head = STX_MALLOC(MEMSZ(type, cap));
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
from (const char* src, size_t len)
{
    const Type type = LEN_TYPE(len);
    void* head = STX_MALLOC(MEMSZ(type, len));
    if (!head) return NULL;

    HSETPROPS(head, type, len, len);

    Attr* attr = HATTR(head,type);
    char* data = attr->data; //DATA(head,type);

    attr->canary = MAGIC;
    attr->flags = type;
    memcpy (data, src, len); //optim cpy len+1 ?
    data[len] = 0; 

    return data;
}

static inline stx_t 
from_to (const char* src, size_t len, char* head)
{
    const Type type = LEN_TYPE(len);
    Attr* attr = HATTR(head,type);
    char* data = attr->data;

    HSETPROPS(head, type, len, len);
    attr->canary = MAGIC;
    attr->flags = type;
    memcpy (data, src, len); //optim cpy len+1 ?
    data[len] = 0; 

    return data;
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
stx_from_len (const void* src, size_t len)
{
    return src ? from(src, len) : new(0);
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
        #if STX_WARNINGS > 0
            ERR ("stx_free: invalid header\n");
        #endif
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
stx_append (stx_t* dst, const char* src, size_t len)
{
    return append((void*)dst, src, len, true);        
}

int 
stx_append_strict (stx_t dst, const char* src, size_t len) 
{
    return append((void*)dst, src, len, false);       
}

int 
stx_append_format (stx_t dst, const char* fmt, ...) 
{
    if (!CHECK(dst)) return 0;

    va_list args;
    va_start(args, fmt);
    int rc = append_format (dst, fmt, args);            
    va_end(args);

    return rc;
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

bool 
stx_check (stx_t s)
{
    return CHECK(s);
}

void 
dbg (stx_t s, bool deep)
{
    if (!CHECK(s)) ERR("stx_dbg: invalid");

    const void* head = HEAD(s);
    const Type type = TYPE(s);

    #define FMT "cap:%zu len:%zu data:\"%s\"\n"
    #define ARGS (size_t)(h->cap), (size_t)(h->len), s
    #define FMT_DEEP "cap:%zu len:%zu canary:%x flags:%x data:\"%s\"\n"
    #define ARGS_DEEP (size_t)(h->cap), (size_t)(h->len), CANARY(s), FLAGS(s), s

    switch(type){
        case TYPE4: {
            Prop4* h = (Prop4*)head;
            if (deep) printf(FMT_DEEP, ARGS_DEEP); else printf(FMT, ARGS);
            break;
        }
        case TYPE1: {
            Prop1* h = (Prop1*)head;
            if (deep) printf(FMT_DEEP, ARGS_DEEP); else printf(FMT, ARGS);
            break;
        }
        default: ERR("stx_dbg: unknown type\n");
    }

    #undef FMT
    #undef ARGS
    #undef FMT_DEEP
    #undef ARGS_DEEP

    fflush(stdout);
}

void stx_dbg (stx_t s) {dbg(s,false);}
void stx_dbg_deep (stx_t s) {dbg(s,true);}


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
    const char *end = strstr(src,sep);//todo strnstr

    while (end) {

        if (cnt >= listmax-2) { // -1 : for last part
            

            if (list == list_local) {

                list_reloc = list_pool;
                listmax = STX_POOL_MAX;

            } else if (list == list_pool) {

                // LOG("dyn");
                listmax *= 2;
                list_dyn = STX_MALLOC (listmax * sizeof(stx_t)); 
                if (!list_dyn) {cnt = 0; goto fin;}
                list_reloc = list_dyn;
            
            } else { // list_dyn
                listmax *= 2;
                list = STX_REALLOC (list, listmax * sizeof(stx_t)); 
                if (!list) {cnt = 0; goto fin;}
            }
        
            if (list_reloc) {
                // LOG("reloc");
                memcpy (list_reloc, list, cnt * sizeof(stx_t));
                list = list_reloc;
                list_reloc = NULL;
            }
        }

        list[cnt++] = from(beg, end-beg);

        end += seplen;
        beg = end;
        end = strstr(end,sep);
    };
    
    list[cnt++] = from(beg, (src+srclen)-beg);

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