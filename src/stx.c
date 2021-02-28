#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "stx.h"
#include "log.h"
#include "util.c"

enum Type {TYPE1=1, TYPE4=2};

#define MAGIC 170 // 0xaa 10101010
#define TYPE_BITS 2
#define TYPE_MASK ((1<<TYPE_BITS)-1)

#define HEAD(s,T) ((Head##T*)(s - sizeof(Attr) - sizeof(Head##T)))
#define HEADV(s,T) (*HEAD(s,T))
#define ATTR(s,name) ((Attr*)((s) - sizeof(Attr)))->name
#define CHECK(s) (ATTR(s,cookie) == MAGIC)
#define FLAGS(s) ATTR(s,flags)
#define TYPE(s) (FLAGS(s) & TYPE_MASK)
#define DATA(s) ATTR(s,data)

typedef struct Head1 {   
    uint8_t     cap;  
    uint8_t     len; 
} Head1;

typedef struct Head4 {   
    uint32_t    cap;  
    uint32_t    len; 
} Head4;

typedef struct Attr {   
    uint8_t     cookie; 
    uint8_t     flags;
    char        data[]; 
} Attr;


stx_t 
stx_new (const size_t cap)
{
    Attr* attr;

    #define INIT(T) \
    Head##T* head = STX_MALLOC(sizeof(*head) + sizeof(Attr) + cap+1); \
    if (!head) return NULL;\
    *head = (Head##T){cap,0}; \
    attr = (Attr*)((char*)head + sizeof(*head)); \
    *attr = (Attr){MAGIC,TYPE##T};

    if (cap < 256) {
        INIT(1); 
    } else {
        INIT(4); 
    }
    #undef INIT

    attr->data[0] = 0; 
    attr->data[cap] = 0; 
    
    return attr->data;
}


void 
stx_free (const stx_t s)
{
    if (!CHECK(s)) {
        ERR ("stx_free: invalid header\n");
        return;
    }

    void* head = NULL;

    int type = TYPE(s);
    
    switch(type){
        case TYPE1: head = HEAD(s,1); *((Head1*)head) = (Head1){0}; break;
        case TYPE4: head = HEAD(s,4); *((Head4*)head) = (Head4){0}; break;
    }

    STX_FREE(head);
}

#define ACCESSOR(s,m) \
if (!CHECK(s)) return 0; \
switch(TYPE(s)){ \
    case TYPE1: return HEAD(s,1)->m; \
    case TYPE4: return HEAD(s,4)->m; \
} \
return 0

size_t 
stx_cap (const stx_t s)
{
    ACCESSOR(s,cap);
}

size_t 
stx_len (const stx_t s)
{
    ACCESSOR(s,len);
}


void 
stx_show (const stx_t s)
{
    if (!CHECK(s)) {
        ERR ("stx_show: invalid header\n");
        return;
    }

    int type = TYPE(s);

    #define SHOW_FMT "cap:%zu len:%zu cookie:%d flags:%d data:'%s'\n"
    #define SHOW_ARGS \
    (size_t)((head)->cap), \
    (size_t)((head)->len), \
    ATTR(s,cookie), \
    ATTR(s,flags), \
    ATTR(s,data)

    switch(type){
        case TYPE1: {
            Head1* head = HEAD(s,1);
            printf (SHOW_FMT, SHOW_ARGS);
            break;
        }
        case TYPE4: {
            Head4* head = HEAD(s,4);
            printf (SHOW_FMT, SHOW_ARGS);
            break;
        }
        default: ERR("stx_show: unknown type\n");
    }

    fflush(stdout);
}

int 
stx_append_count (const stx_t dst, const char* src, const size_t n) 
{
    if (!src) 
        return STX_FAIL;

    const int type = TYPE(dst);
    char* dst_data = DATA(dst);
    size_t dst_cap, dst_len;
    
    switch(type) {
        case TYPE1: {
            Head1 dsthv = HEADV(dst,1);
            dst_cap = dsthv.cap;
            dst_len = dsthv.len;
            break;
        }
        case TYPE4: {
            Head4 dsthv = HEADV(dst,4);
            dst_cap = dsthv.cap;
            dst_len = dsthv.len;
            break;
        }
    }

    char* dst_end = dst_data + dst_len;
    const size_t inc_len = n ? strnlen(src,n) : strlen(src);

    // Would truncate - return total needed capacity
    if (inc_len > dst_cap - dst_len)
        return -(dst_len + inc_len);
    
    memcpy (dst_end, src, inc_len);
    *(dst_end + inc_len) = 0;

    switch(type) {
        case TYPE1: {
            HEAD(dst,1)->len += inc_len;
            break;
        }
        case TYPE4: {
            HEAD(dst,4)->len += inc_len;
            break;
        }
    }

    return inc_len;           
}