/*
Stricks v0.5.0
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
    uint8_t flags;
    char data[]; 
} Attr;

// comptime math : not with clang ?
typedef enum {
    TYPE1 = 1, // (int)log2(sizeof(Head1)), 
    TYPE4 = 3  // (int)log2(sizeof(Head4))
} Type;

#define TYPE_BITS 2
#define TYPE_MASK ((1<<TYPE_BITS)-1)
#define SMALL_MAX 255 // max TYPE1 capacity

#define HEADSZ(type) (1<<type)
#define DATAOFF(type) (HEADSZ(type) + offsetof(Attr,data))
#define TOTSZ(type,cap) (DATAOFF(type) + cap + 1)

#define FLAGS(s) (((uint8_t*)(s))[-1])
#define TYPE(s) (FLAGS(s) & TYPE_MASK)
#define HEAD(s) ((char*)(s) - DATAOFF(TYPE(s)))
#define HEADT(s,type) ((char*)(s) - DATAOFF(type))
#define DATA(head,type) ((char*)(head) + DATAOFF(type))

#define TYPE_FOR(len) ((len <= SMALL_MAX) ? TYPE1 : TYPE4)

#define LIST_LOCAL_MAX (STX_LOCAL_MEM/sizeof(stx_t))
#define LIST_POOL_MAX (STX_POOL_MEM/sizeof(stx_t))

static_assert (DATAOFF(TYPE1)==3, "bad DATAOFF");
static_assert (DATAOFF(TYPE4)==9, "bad DATAOFF");

//==== PRIVATE =================================================================

stx_t list_pool[LIST_POOL_MAX] = {NULL};

#define head_getter(prop) \
static inline size_t hget##prop (const void* head, Type type) { \
    switch(type) { \
        case TYPE4: return ((Head4*)head)->prop; \
        case TYPE1: return ((Head1*)head)->prop; \
    }  \
}

head_getter(cap)
head_getter(len)

#define head_setter(prop) \
static inline void hset##prop (const void* head, Type type, size_t val) { \
    switch(type) { \
        case TYPE4: ((Head4*)head)->prop = val; break; \
        case TYPE1: ((Head1*)head)->prop = val; break; \
        default: ERR("Bad head type"); exit(1); \
    } \
} 

head_setter(cap) // hsetcap 
head_setter(len) // hsetlen

static inline Head4
hgetdims (const void* head, Type type)
{
    switch(type) {
        case TYPE4: return *(Head4*)head;
        case TYPE1: return (Head4){((Head1*)head)->cap, ((Head1*)head)->len};
    } 
}

static inline void
hsetdims (const void* head, Type type, Head4 dims)
{
    switch(type) {
        case TYPE4: *((Head4*)head) = (Head4)dims; break;
        case TYPE1: *((Head1*)head) = (Head1){dims.cap, dims.len}; break;
    } 
}

static inline size_t
getlen (stx_t s)
{
    const Type type = TYPE(s);
    return hgetlen(HEADT(s,type), type); //opt ?
}

static inline void
setlen (stx_t s, size_t len)
{
    const Type type = TYPE(s);
    void* head = HEADT(s, type);
    hsetlen(head, type, len);
}

static inline size_t 
getspc (stx_t s)
{
    const Type type = TYPE(s);
    void* head = HEADT(s, type);
    switch(type) { 
        case TYPE4: return ((Head4*)head)->cap - ((Head4*)head)->len;
        case TYPE1: return ((Head1*)head)->cap - ((Head1*)head)->len;
    }  
}


static inline stx_t 
new (size_t cap)
{
    const Type type = TYPE_FOR(cap);
    void* head = STX_MALLOC(TOTSZ(type, cap));
    if (!head) return NULL;

    hsetdims(head, type, (Head4){cap, 0});

    char* data = DATA(head,type);
    data[0] = 0; 
    data[cap] = 0; 

    FLAGS(data) = type;
    
    return data;
}

static inline stx_t 
from (const char* src, size_t srclen)
{
    const Type type = TYPE_FOR(srclen);
    void* head = STX_MALLOC(TOTSZ(type, srclen));
    if (!head) return NULL;

    hsetdims(head, type, (Head4){srclen, srclen});

    char* data = DATA(head,type);
    memcpy (data, src, srclen);
    data[srclen] = 0; 

    FLAGS(data) = type;

    return data;
}


static inline const void* 
resize (stx_t *ps, size_t newcap)
{    
    stx_t s = *ps;

    const Type type = TYPE(s);
    const void* head = HEADT(s, type);
    Head4 dims = hgetdims(head,type);

    if (newcap == dims.cap) return head;
    // newcap < cap : optim realloc only if < 1/2 ?

    const Type newtype = TYPE_FOR(newcap);
    const int sametype = (newtype == type);
    const size_t newsize = TOTSZ(newtype, newcap);
    
    void* newhead = sametype ? STX_REALLOC((void*)head, newsize)
                             : STX_MALLOC(newsize);

    if (!newhead) {
        ERR ("stx_resize: realloc");
        return NULL;
    }
    
    char* newdata = DATA(newhead, newtype);
    const size_t newlen = min(dims.len, newcap);
    
    if (!sametype) {
        // copy data
        memcpy (newdata, s, newlen); 
        newdata[newlen] = 0; //nec?
        // update type
        FLAGS(newdata) = /*(FLAGS(newdata) & ~TYPE_MASK) |*/ newtype;
        free((void*)head);
    }
    
    hsetdims (newhead, newtype, (Head4){newcap, newlen});
    newdata[newcap] = 0;
    
    *ps = newdata;
    return newhead;
}


// resize increase only, to new location
static inline void* 
grow (stx_t *ps, size_t newcap, const void* head, Type type, Head4 dims)
{    
    stx_t s = *ps;
    void* newhead;
    char* newdata;

    #define RELOC(t) \
        newhead = STX_REALLOC((void*)head, TOTSZ(TYPE##t, newcap)); \
        if (!newhead) {ERR("realloc"); return NULL;} \
        ((Head##t*)newhead)->cap = newcap; \
        newdata = DATA(newhead, TYPE##t)

    // TYPE4 -> TYPE4
    if (type == TYPE4) {
        RELOC(4); goto fin;
    }

    // TYPE1 -> TYPE1
    if (newcap <= SMALL_MAX) {
        RELOC(1); goto fin;
    }

    #undef RELOC

    // TYPE1 -> TYPE4
    newhead = STX_REALLOC((void*)head, TOTSZ(TYPE4, newcap));// todo err
    newdata = DATA(newhead, TYPE4);
    memmove (newdata, DATA(newhead, TYPE1), dims.len+1); 
    *((Head4*)newhead) = (Head4){newcap, dims.len};
    // ((Head4*)newhead)->len = dims.len;
    FLAGS(newdata) = TYPE4;

    fin:
    newdata[newcap] = 0;
    *ps = newdata;

    return newhead;
}



// copy only up to current length
static stx_t
dup (stx_t src)
{
    const Type type = TYPE(src);
    const void* head = HEADT(src, type);
    const size_t len = hgetlen(head, type);
    const size_t cpysz = TOTSZ(type,len);
    void* new_head = malloc(cpysz);

    if (!new_head) return NULL;

    memcpy (new_head, head, cpysz);
    hsetcap (new_head, type, len);
    stx_t ret = DATA(new_head, type);
    ((char*)ret)[len] = 0;

    return ret;
}

//==== PUBLIC ==================================================================

size_t 
stx_append (stx_t* dst, const void* src, size_t srclen) 
{
    stx_t s = *dst;
    
    const Type type = TYPE(s);
    void* head = HEADT(s, type);
    const Head4 dims = hgetdims(head,type);
    const size_t totlen = dims.len + srclen;

    if (totlen > dims.cap) {  

        head = grow (dst, totlen*2, head, type, dims);
        
        if (!head) {
            ERR("resize");
            return 0;
        }

        s = *dst;
    }

    char* end = (char*)s + dims.len;

    memcpy (end, src, srclen);
    end[srclen] = 0;
    hsetlen (head, TYPE(s), totlen);

    return totlen;              
}


long long 
stx_append_strict (stx_t dst, const void* src, size_t srclen) 
{
    const Type type = TYPE(dst);
    void* head = HEADT(dst, type);
    const Head4 dims = hgetdims(head,type);
    const size_t totlen = dims.len + srclen;

    // Would truncate, return needed capacity
    if (totlen > dims.cap) {  
        return -totlen;
    }

    char* end = (char*)dst + dims.len;
    memcpy (end, src, srclen);
    end[srclen] = 0;

    hsetlen (head, type, totlen);

    return totlen;        

}



size_t 
stx_append_fmt (stx_t* dst, const char* fmt, ...) 
{
    stx_t s = *dst;

    const Type type = TYPE(s);
    const void* head = HEADT(s,type);
    const Head4 dims = hgetdims(head,type);
    char local[STX_LOCAL_MEM];

    va_list args, argscpy;
    va_start(args, fmt);  
    va_copy(argscpy, args); 

    errno = 0;
    const int fmtlen = vsnprintf (local, sizeof(local), fmt, args);
    va_end(args);

    if (fmtlen < 0) {
        perror("vsnprintf"); 
        return 0;
    }
    
    const size_t totlen = dims.len + fmtlen;

    if (totlen > dims.cap) {

        if (!grow(dst, 2*totlen, head, type, dims)) {
            ERR("resize");
            return 0;
        }
        
        s = *dst;
    } 
    
    char* end = (char*)s + dims.len;

    if (fmtlen < STX_LOCAL_MEM) {
        memcpy (end, local, fmtlen);
    } else {
        vsprintf (end, fmt, argscpy);
        va_end(argscpy);
    }

    end[fmtlen] = 0;
    setlen(s, totlen);

    return totlen;           
}


long long 
stx_append_fmt_strict (stx_t dst, const char* fmt, ...) 
{
    const Type type = TYPE(dst);
    const void* head = HEADT(dst, type);
    const Head4 dims = hgetdims(head,type);
    const size_t spc = dims.cap - dims.len;
    char* end = (char*)dst + dims.len;

    if (!spc) return 0; // old len ?

    va_list args;
    va_start(args, fmt);   
    errno = 0;
    const int fmtlen = vsnprintf (end, spc+1, fmt, args);
    va_end(args);

    if (fmtlen < 0) {
        perror("vsnprintf");
        *end = 0; //undo
        return 0;
    }

    const size_t totlen = dims.len + fmtlen;
     
    // Truncation
    if (totlen > dims.cap) {
        *end = 0; //undo
        return -totlen;            
    }     

    // Update length
    hsetlen(head, type, totlen);

    return totlen;           
}


stx_t*
stx_split_len (const char* src, size_t srclen, const char* sep, size_t seplen, int* outcnt)
{
    int cnt = 0; 
    stx_t* ret = NULL;
    
    // rem: strstr(s,"") == s
    if (!seplen) goto fin;

    stx_t  list_local[LIST_LOCAL_MAX]; 
    stx_t *list_dyn = NULL;
    stx_t *list_reloc = NULL;
    stx_t *list = list_local;
    int listmax = LIST_LOCAL_MAX;

    const char *beg = src;
    const char *end = src;

    while ((end = strstr(end, sep))) {

        if (cnt >= listmax-2) { // -2 : last part + sentinel

            if (list == list_local) {

                list_reloc = list_pool;
                listmax = LIST_POOL_MAX;

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


void
stx_list_free (const stx_t *list)
{
    const stx_t *l = list;
    stx_t s;
    while ((s = *l++)) STX_FREE(HEAD(s)); 
    STX_FREE((void*)list);
}


stx_t 
stx_join_len (stx_t *list, int count, const char* sep, size_t seplen)
{
    size_t totlen = 0;

    for (int i = 0; i < count; ++i)
        totlen += getlen(list[i]);
    totlen += (count-1)*seplen;
    
    stx_t ret = new(totlen);
    char* cur = (char*)ret;

    for (int i = 0; i < count-1; ++i) {
        stx_t elt = list[i];
        const size_t eltlen = getlen(elt);
        memcpy(cur, elt, eltlen);
        cur += eltlen;
        memcpy(cur, sep, seplen);
        cur += seplen;
    }

    stx_t last = list[count-1];
    memcpy(cur, last, getlen(last));

    setlen(ret, totlen);
    return ret;
}


// todo new fit type ?
void 
stx_trim (stx_t s)
{
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


// nb: memcmp(,,0) == 0
int 
stx_equal (stx_t a, stx_t b) 
{
    const size_t lena = getlen(a);
    const size_t lenb = getlen(b);
    return (lena == lenb) && !memcmp(a, b, lena);
}


void
stx_dbg (stx_t s)
{
    #define DBGFMT "cap:%zu len:%zu flags:%x data:\"%s\"\n"
    #define DBGARG (size_t)(h->cap), (size_t)(h->len), FLAGS(s), s

    const void* head = HEAD(s);
    const Type type = TYPE(s);

    switch(type){
        case TYPE4: {
            Head4* h = (Head4*)head;
            printf(DBGFMT, DBGARG);
            break;
        }
        case TYPE1: {
            Head1* h = (Head1*)head;
            printf(DBGFMT, DBGARG);
            break;
        }
        default: ERR("stx_dbg: unknown type");
    }

    #undef DBGFMT
    #undef DBGARG
    fflush(stdout);
}
//==== WRAPPERS & CONVENIENCES =====

stx_t stx_new (size_t cap) {
    return new(cap);
}

stx_t stx_from (const char* src) {
    return from(src, strlen(src));
}

stx_t stx_from_len (const void* src, size_t srclen) {
    return from(src, srclen);
}

stx_t stx_dup (stx_t src) {
    return dup(src);
}

void stx_free (stx_t s) {
    STX_FREE(HEAD(s));
}

int stx_resize (stx_t *ps, size_t newcap) {
    return !!resize (ps, newcap);
}

stx_t* stx_split (const char* src, const char* sep, int* outcnt) {
    const size_t srclen = sep ? strlen(src) : 0;
    const size_t seplen = sep ? strlen(sep) : 0;
    return stx_split_len (src, srclen, sep, seplen, outcnt);
}

stx_t stx_join (stx_t *list, int count, const char* sep) {
    return stx_join_len (list, count, sep, strlen(sep));
}

size_t stx_cap (stx_t s) {
    return hgetcap(HEAD(s), TYPE(s));
}

size_t stx_len (stx_t s) {
    return getlen(s);
}

size_t stx_spc (stx_t s) {
    return getspc((s));
}

void stx_reset (stx_t s) {
    setlen(s,0);
    *((char*)s) = 0;
} 

void stx_adjust (stx_t s) {
    setlen(s,strlen(s));
}