/*
Stricks v0.3.2
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
    uint8_t canary; 
    uint8_t flags;
    char data[]; 
} Attr;

// clang rejects comptime math ?
typedef enum {
    TYPE1 = 1, // (int)log2(sizeof(Head1)), 
    TYPE4 = 3  // (int)log2(sizeof(Head4))
} Type;

typedef struct {uint32_t off; uint32_t len;} Part; // for split()

static_assert ((1<<TYPE1) == sizeof(Head1), "bad TYPE1");
static_assert ((1<<TYPE4) == sizeof(Head4), "bad TYPE4");

#define MAGIC 0xaa
#define TYPE_BITS 2
#define TYPE_MASK ((1<<TYPE_BITS)-1)
#define SMALL_CAP 255

#define HEAD(s) ((char*)(s) - sizeof(Attr) - TYPESZ(TYPE(s)))
#define ATTR(s) ((Attr*)((char*)(s) - sizeof(Attr)))
#define FLAGS(s)  ATTR(s)->flags
#define CANARY(s) ATTR(s)->canary
#define TYPE(s) (FLAGS(s) & TYPE_MASK)
#define TYPESZ(type) (1<<type)
#define MEMSZ(type,cap) (TYPESZ(type) + sizeof(Attr) + cap + 1) //MIN_CAP ??
#define CHECK(s) ((s) && CANARY(s) == MAGIC)
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

//==== PRIVATE ================================================================

// enforce STX_MIN_CAP ?
static bool 
resize (stx_t *ps, size_t newcap)
{    
    stx_t s = *ps;

    if (!CHECK(s)||!newcap) return false;

    const void* head = HEAD(s);
    const Type type = TYPE(s);
    const size_t cap = HGETCAP(head, type);
    const size_t len = HGETLEN(head, type);

    if (newcap == cap) return true;
    
    const Type newtype = (newcap > SMALL_CAP) ? TYPE4 : TYPE1;
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
        // reput len
        HSETLEN(newhead, newtype, len);
        // reput canary
        CANARY(newdata) = MAGIC;
        // update flags
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


static int 
append (void* dst, const char* src, size_t n, bool alloc) 
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

    char* end = (char*)s + len;
    memcpy (end, src, inc);
    end[inc] = 0;
    SETPROP(s, len, totlen);

    return inc;        
}


static int 
append_format (stx_t dst, const char* fmt, va_list args)
{
    const void* head = HEAD(dst);
    const Type type = TYPE(dst);
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
    if (fmlen > spc) {
        
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
dup (stx_t s)
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
    ((char*)ret)[len] = 0;

    return ret;
}

//==== PUBLIC ==================================================================

stx_t 
stx_new (size_t mincap)
{
    const size_t cap = max(mincap, STX_MIN_CAP);
    const Type type = (cap > SMALL_CAP) ? TYPE4 : TYPE1;

    void* head = STX_MALLOC(MEMSZ(type, cap));
    if (!head) return NULL;

    HSETCAP(head, type, cap);
    HSETLEN(head, type, 0);

    Attr* attr = (Attr*)((char*)(head) + TYPESZ(type));
    attr->canary = MAGIC;
    attr->flags = type;
    attr->data[0] = 0; 
    attr->data[cap] = 0; 
    
    return (const stx_t)(attr->data);
}

stx_t
stx_from (const char* src)
{
    if (!src) return stx_new(STX_MIN_CAP);

    const size_t len = strlen(src);
    stx_t ret = stx_new(len);
    
    memcpy ((char*)ret, src, len);
    SETPROP (ret, len, len);

    return ret;
}

// todo optm strncpy_s ?
stx_t
stx_from_len (const char* src, size_t len)
{
    stx_t dst = stx_new(len);
    if (!src) return dst;
    
    const char* s = src;
    char* d = (char*)dst;
    size_t stock = len;

    for (; *s && stock; --stock) 
        *d++ = *s++;
    
    SETPROP (dst, len, len-stock);

    return dst;
}

stx_t
stx_dup (stx_t s)
{
    return CHECK(s) ? dup(s) : NULL;
}

void 
stx_reset (stx_t s)
{
    if (!CHECK(s)) return;
    SETPROP(s, len, 0);
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

    void* head = HEAD(s);
    
    switch(TYPE(s)) {
        case TYPE4: bzero(head, sizeof(Head4) + sizeof(Attr)); break;
        case TYPE1: bzero(head, sizeof(Head1) + sizeof(Attr)); break;
    }

    STX_FREE(head);
}


size_t 
stx_cap (stx_t s) 
{
    if (!CHECK(s)) return 0;  
    return GETPROP(s, cap);  
}

size_t 
stx_len (stx_t s) 
{
    if (!CHECK(s)) return 0;
    return GETPROP(s, len);
}

size_t 
stx_spc (stx_t s)
{
    if (!CHECK(s)) return 0;
    return GETSPC(s);
}

int 
stx_append (stx_t dst, const char* src) 
{
    return append((void*)dst, src, 0, false);       
}

int 
stx_append_count (stx_t dst, const char* src, size_t n) 
{
    return append((void*)dst, src, n, false);       
}

size_t 
stx_append_alloc (stx_t* dst, const char* src)
{
    return append((void*)dst, src, 0, true);        
}

size_t 
stx_append_count_alloc (stx_t* dst, const char* src, size_t n)
{
    return append((void*)dst, src, n, true);        
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
    return resize(ps, newcap);
}

// nb: memcmp(,,0) == 0
bool 
stx_equal (stx_t a, stx_t b) 
{
    if (!CHECK(a)||!CHECK(b)) return false;

    const size_t lena = GETPROP((a), len);
    const size_t lenb = GETPROP((b), len);

    return (lena == lenb) && !memcmp(a, b, lena);
}

bool 
stx_check (stx_t s)
{
    return CHECK(s);
}

void 
stx_show (stx_t s)
{
    if (!CHECK(s)) {
        ERR("stx_show: invalid");
        return;
    }

    const void* head = HEAD(s);
    const Type type = TYPE(s);

    #define FMT "%zu %zu %x %x \"%s\"\n"
    #define ARGS (size_t)(h->cap), (size_t)(h->len), CANARY(s), FLAGS(s), s

    switch(type){
        case TYPE4: {
            Head4* h = (Head4*)head;
            printf (FMT, ARGS);
            break;
        }
        case TYPE1: {
            Head1* h = (Head1*)head;
            printf (FMT, ARGS);
            break;
        }
        default: ERR("stx_show: unknown type\n");
    }

    #undef FMT
    #undef ARGS

    fflush(stdout);
}


// todo new fit type ?
void 
stx_trim (stx_t s)
{
    if (!CHECK(s)) return;
    
    const char* front = s;
    while (isspace(*front)) ++front;

    const char* end = s + GETPROP(s,len);
    while (end > front && isspace(*(end-1))) --end;
    
    const size_t newlen = end-front;
    
    if (front > s) {
        memmove((void*)s, front, newlen);
    }
    
    ((char*)s)[newlen] = 0;
    SETPROP(s, len, newlen);
}


void
stx_update (stx_t s)
{
    if (!CHECK(s)) return;
    SETPROP(s, len, strlen(s));
}


stx_t*
stx_split (const void* src, size_t srclen, const char* sep, 
    unsigned int* outcnt)
{
    if (!src) {
        *outcnt = 0;
        return NULL;
    }
    
    const size_t seplen = sep ? strlen(sep) : 0;
    
    // solid ?
    const unsigned nparts = str_count(src,sep)+1;
    Part parts[nparts];

    const char* s = src;
    const char* beg = s;
    unsigned cnt = 0;
    // size_t blocksz = 0;
    stx_t* list;

    if (!seplen) goto last;

    const char *end = strstr(s,sep);

    while (end) {
        const size_t len = end-beg;
        // Type type = len <= SMALL_CAP ? TYPE1 : TYPE4;
        // size_t partsz = MEMSZ(type,len);
        // blocksz += partsz;
        parts[cnt++] = (Part){beg-s, len};
        end += seplen;
        beg = end;
        end = strstr(end,sep);
    };

    last:
    parts[cnt++] = (Part){beg-s, s+srclen-beg}; //bof

    // char* block = STX_MALLOC(blocksz);
    list = STX_MALLOC((cnt+1) * sizeof(*list)); // +1: sentinel

    for (int i = 0; i < cnt; ++i)
    {   
        Part part = parts[i];
        list[i] = stx_from_len (s + part.off, part.len);
    }

    list[cnt] = NULL; // sentinel
    *outcnt = cnt;
    
    return list;
}


// implies sentinel !
void
stx_list_free (const stx_t* list)
{
    const stx_t* l = list;
    stx_t s;

    while (s = *l++) stx_free(s);
    
    free((void*)list);
}