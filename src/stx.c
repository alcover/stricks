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

#define BLOCK_SZ(type,cap) (sizeof(type) + (cap) + 1)
#define HEAD(s) ((Long*)((s) ? ((char*)(s) - offsetof(Long,data)) : NULL))
#define FIELD_ADR(s,name) ((char*)(s) - offsetof(Long,data) + offsetof(Long,name))
#define FIELD_VAL(s,name) (*FIELD_ADR((s),name))

#define COOKIE 147

struct Long {   
    size_t  cap;  
    size_t  len; 
    uint8_t cookie; 
    uint8_t type;
    char    data[]; 
};

typedef struct Long Long;

struct Short {   
    uint8_t cap;  
    uint8_t len; 
    uint8_t cookie; 
    uint8_t type;
    char    data[]; 
};

typedef struct Short Short;

//==== PRIVATE ================================================================

// duck-validate
static inline bool
check (const Long* head)
{
    return \
        head                        
    &&  head->cookie == COOKIE      
    &&  (intmax_t)head->cap >= 0    // ?
    &&  (intmax_t)head->len >= 0    // ?
    &&  head->cap >= head->len;      
}


static bool 
resize (void **phead, const size_t newcap)
{    
    // negative value - abort
    if ((intmax_t)newcap < 0) {
        #ifdef STX_WARNINGS
            ERR("stx_resize: negative value\n");
        #endif
        return false;
    }


    void* head = *phead;
    
    if (newcap == head->cap) 
        return true;
    
    void* tmp = realloc (head, BLOCK_SZ(newcap));    

    if (!tmp) {
        ERR ("stx_resize: realloc failed\n");
        return false;
    }
    
    // truncated
    if (newcap < tmp->len) {
        #ifdef STX_WARNINGS
            LOG ("stx_resize: truncated");
        #endif
        tmp->len = newcap;
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

#define TYPE_LONG 0
#define TYPE_SHORT 1

static stx_t 
new_short (const size_t cap)
{
    Short* head = STX_MALLOC (BLOCK_SZ(Short,cap));

    if (!head) return NULL;
    
    *head = (Short){0}; // sec
    
    head->cap = cap;
    head->cookie = COOKIE;
    head->type = TYPE_SHORT;
    head->data[0] = 0;
    head->data[cap] = 0;

    return head->data;
}

static stx_t 
new_long (const size_t cap)
{
    Long* head = STX_MALLOC (BLOCK_SZ(Long,cap));

    if (!head) return NULL;
    
    *head = (Long){0}; // sec
    
    head->cap = cap;
    head->cookie = COOKIE;
    head->type = TYPE_LONG;
    head->data[0] = 0;
    head->data[cap] = 0;

    return head->data;
}


const stx_t 
stx_new (const size_t cap)
{
    return (cap < 256) ? new_short(cap) : new_long(cap);    
}


void 
stx_free (const stx_t s)
{
    void* head = HEAD(s);

    if (!check(head)) {
        #ifdef STX_WARNINGS
            ERR ("stx_free: invalid header\n");
        #endif
        return;
    }

    *head = (Long){0};
    STX_FREE(head);
}


bool
stx_resize (stx_t *pstx, const size_t newcap)
{
    void* head = HEAD(*pstx);

    if (!check(head)) 
        return false;

    bool resized = resize(&head, newcap);

    *pstx = head->data;

    return resized;
}


int 
stx_append_count (const stx_t dst, const char* src, const size_t n) 
{
    void* head = HEAD(dst);

    if (!check(head)) 
        return STX_FAIL;

    return append_count (head, src, n);            
}

size_t 
stx_cap (const stx_t s)
{
    return check(HEAD(s)) ? FIELD_VAL(s,cap) : 0;
}

size_t 
stx_len (const stx_t s)
{
    return check(HEAD(s)) ? FIELD_VAL(s,len) : 0;
}

void 
stx_show (const stx_t s)
{
    const Long* head = HEAD(s);

    if (!check(head)) {
        ERR ("stx_show: invalid header\n");
        return;
    }

    printf ("cap:%zu len:%zu data:'%s'\n",
        (size_t)head->cap, (size_t)head->len, head->data);

    fflush(stdout);
}