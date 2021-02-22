/*
Stricks v0.1.0
Copyright (C) 2021 - Francois Alcover <francois@alcover.fr>
NO WARRANTY EXPRESSED OR IMPLIED.
*/

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

typedef unsigned char uchar;

#define TYPE_8  8
#define TYPE_32 32 

#define BLOCK_SZ(T,cap) (sizeof(Head##T) + (cap) + 1)
#define HEAD(s,T) ((s) - offsetof(Head##T,data))
#define FIELD_ADR(s,htype,name) ((char*)(s) - offsetof(htype,data) + offsetof(htype,name))
#define FIELD_VAL(s,htype,name) (*FIELD_ADR((s),htype,name))
#define CHECK(s) ((s) && (((char*)(s))[-2] == COOKIE))
#define TYPE(s) (((char*)(s))[-1])

#define COOKIE 147

typedef struct  {   
    uint8_t cap;  
    uint8_t len; 
    char    cookie; 
    uchar   type;
    char    data[]; 
} Head8;

typedef struct  {   
    uint32_t    cap;  
    uint32_t    len; 
    char        cookie; 
    uchar       type;
    char        data[]; 
} Head32;

//==== PRIVATE ================================================================

void* headof(const stx_t s, uchar* outtype)
{
    char type = s[-1];
    int off;

    if (type == TYPE_8) {
        off = offsetof(Head8, data);
    } else {
        off = offsetof(Head32, data);
    }

    *outtype type;
    return s - off;
}


static inline bool
check (const void* head)
{
    return head && (((char*)head)[-2] == COOKIE);
}

static inline size_t
get_cap (const void* head, char type)
{
    if (type == TYPE_8)
        return ((Head8*)head)->cap;
    else
        return ((Head32*)head)->cap;
}


static bool 
resize (void **phead, char type, const size_t newcap)
{    
    // negative value - abort
    if ((intmax_t)newcap < 0) {
        #ifdef STX_WARNINGS
            ERR("stx_resize: negative value\n");
        #endif
        return false;
    }


    void* head = *phead;
    void* tmp;
    
    if (newcap == get_cap(head,type)) 
        return true;

    if (type == TYPE_8)
        tmp = realloc (head, BLOCK_SZ(8,newcap)); 
    else
        tmp = realloc (head, BLOCK_SZ(32,newcap));    

    if (!tmp) {
        ERR ("stx_resize: realloc failed\n");
        return false;
    }
    
    // truncated
    if (newcap < tmp->len) {

        #ifdef STX_WARNINGS
            LOG ("stx_resize: truncated");
        #endif

        if (type == TYPE_8)
            ((Head8*)tmp)->len = newcap;
        else
            ((Head32*)tmp)->len = newcap;
    }

    // set new cap sentinel
    tmp->data[newcap] = 0;
    // update cap
    tmp->cap = newcap;
    *phead = tmp;
    
    return true;
}


static int 
append_count (void* dst, const char* src, const size_t n) 
{
    if (!src) 
        return STX_FAIL;

    const size_t dst_len = dst->len;
    char* dst_end = dst->data + dst_len;
    const size_t inc_len = n ? strnlen(src,n) : strlen(src);

    // Would truncate - return total needed capacity
    if (inc_len > dst->cap - dst_len)
        return -(dst_len + inc_len);
    
    memcpy (dst_end, src, inc_len);
    *(dst_end + inc_len) = 0;
    dst->len += inc_len;

    return inc_len;            
} 

//==== PUBLIC ================================================================

#define NEW(head, cap, T) \
    void* head = STX_MALLOC (BLOCK_SZ(T,cap)); \
    if (!head) return NULL; \
    ((Head##T*)head)->cap = cap; \
    ((Head##T*)head)->cookie = COOKIE; \
    ((Head##T*)head)->type = T; \
    ((Head##T*)head)->data[0] = 0; \
    ((Head##T*)head)->data[cap] = 0; \
    return ((Head##T*)head)->data

const stx_t 
stx_new (const size_t cap)
{
    void* head;

    if (cap < 256) {
        NEW (head, cap, 8);
    } else {
        NEW (head, cap, 32);
    }
}


void 
stx_free (const stx_t s)
{
    if (!CHECK(s)) {
        ERR ("stx_free: invalid header\n");
        return;
    }

    uchar type;
    void* head = headof(s);
    
    if (type == TYPE_8) {
        *(Head8*)head = (Head8){0};
    } else {
        *(Head32*)head = (Head32){0};
    }

    STX_FREE(head);
}


bool
stx_resize (stx_t *pstx, const size_t newcap)
{
    stx_t s = *pstx;

    if (!CHECK(s)) return false;

    void* head = headof(*pstx);
    char type = TYPE(s);

    bool resized = resize (&head, type, newcap);
    
    if (type == TYPE_8) {
        *pstx = ((Head8*)head)->data;
    } else {
        *pstx = ((Head32*)head)->data;
    }

    return resized;
}


int 
stx_append_count (const stx_t dst, const char* src, const size_t n) 
{
    void* head = headof(dst);

    if (!check(head)) 
        return STX_FAIL;

    return append_count (head, src, n);            
}

size_t 
stx_cap (const stx_t s)
{
    size_t ret;
    char type = s[-1];
    
    if (type == TYPE_8) {
        ret = *((uint8_t*)(s - offsetof(Head8,data) + offsetof(Head8,cap)));
    } else {
        ret = *((size_t*)(s - offsetof(Head32,data) + offsetof(Head32,cap)));
    }

    return ret;
}

size_t 
stx_len (const stx_t s)
{
    size_t ret;
    char type = s[-1];

    if (type == TYPE_8) {
        ret = *((uint8_t*)(s - offsetof(Head8,data) + offsetof(Head8,len)));
    } else {
        ret = *((size_t*)(s - offsetof(Head32,data) + offsetof(Head32,len)));
    }

    return ret;
}


void 
stx_show (const stx_t s)
{
    const void* head = headof(s);

    if (!check(head)) {
        ERR ("stx_show: invalid header\n");
        return;
    }

    printf ("cap:%zu len:%zu data:'%s'\n",
        (size_t)head->cap, (size_t)head->len, head->data);

    fflush(stdout);
}