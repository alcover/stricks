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

#define BLOCK_SZ(cap) (sizeof(Header) + (cap) + 1)
#define HEAD(s) ((Header*)((s) ? ((char*)(s) - offsetof(Header,data)) : NULL))
#define FIELD_ADR(s,name) ((char*)(s) - offsetof(Header,data) + offsetof(Header,name))
#define FIELD_VAL(s,name) (*FIELD_ADR((s),name))

#define COOKIE 147

struct Header {   
    size_t  cap;  
    size_t  len; 
    uint8_t cookie; 
    char    data[]; 
};

typedef struct Header Header;

struct SSO {   
    uint8_t  cap;  
    uint8_t  len; 
    uint8_t cookie; 
    char    data[]; 
};

//==== PRIVATE ================================================================

// duck-validate
static inline bool
check (const Header* head)
{
    return \
        head                        
    &&  head->cookie == COOKIE      
    &&  (intmax_t)head->cap >= 0    // ?
    &&  (intmax_t)head->len >= 0    // ?
    &&  head->cap >= head->len;      
}


static bool 
resize (Header **phead, const size_t newcap)
{    
    // negative value - abort
    if ((intmax_t)newcap < 0) {
        #ifdef STX_WARNINGS
            ERR("stx_resize: negative value\n");
        #endif
        return false;
    }


    Header* head = *phead;
    
    if (newcap == head->cap) 
        return true;
    
    Header* tmp = realloc (head, BLOCK_SZ(newcap));    

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
append_count (Header* dst, const char* src, const size_t n) 
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


static size_t 
append_count_alloc (Header **pdst, const char* src, const size_t n)
{
    Header* head = *pdst;

    if (!head) 
        return STX_FAIL;

    const size_t dst_len = head->len;
    char* dst_end = head->data + dst_len;
    const size_t inc_len = n ? strnlen(src,n) : strlen(src);
    const size_t min_cap = dst_len + inc_len;

    if (min_cap > head->cap && ! resize(&head, min_cap*2)) 
        return STX_FAIL;
    
    memcpy (dst_end, src, inc_len);
    *(dst_end+inc_len) = 0;
    head->len += inc_len; // solid ?

    return inc_len;
}


static int 
append_format (Header* dst, const char* fmt, va_list args)
{
    const size_t dst_len = dst->len;
    const size_t spc = dst->cap - dst_len;
    
    if (!fmt||!spc) 
        return STX_FAIL;

    char* dst_end = dst->data + dst_len;

    errno = 0;
    const int src_len = vsnprintf (dst_end, spc+1, fmt, args);
     
    // Error
    if (src_len < 0) {
        perror("stx_append_format");
        *dst_end = 0; // undo
        return STX_FAIL;
    }

    // Truncation
    if (src_len > spc) {
        *dst_end = 0; // undo
        return -(dst_len + src_len);
    } 

    // Update length
    dst->len += src_len;

    return src_len;
}


static Header*
dup (const Header* src)
{
    const size_t len = src->len;
    const size_t sz = BLOCK_SZ(len);
    Header* ret = malloc(sz);

    if (!ret)
        return NULL;

    memcpy (ret, src, sz);
    ret->cap = len;
    ret->data[len] = 0; // to be sure

    return ret;
}

//==== PUBLIC ================================================================

/*
Allocates and inits a new `strick` of capacity `cap`.
*/
const stx_t 
stx_new (const size_t cap)
{
    Header* head = STX_MALLOC(BLOCK_SZ(cap));

    if (!head) return NULL;

    *head = (Header){0}; // sec

    head->cap = cap;
    head->len = 0;
    head->cookie = COOKIE;
    head->data[0] = 0;
    head->data[cap] = 0;
    
    return head->data;
}


void 
stx_free (const stx_t s)
{
    Header* head = HEAD(s);

    if (!check(head)) {
        #ifdef STX_WARNINGS
            ERR ("stx_free: invalid header\n");
        #endif
        return;
    }

    *head = (Header){0};
    STX_FREE(head);
}


/*
Creates a new strick and copies `src` to it.  
Capacity gets trimmed down to length.
*/
// prototype
const stx_t
stx_from (const char* src)
{
    const size_t len = strlen(src);

    const stx_t ret = stx_new(len);
    stx_append_count (ret, src, len);

    return ret;
}


/*
Creates a duplicate strick of `src`.  
Capacity gets trimmed down to length.
*/
const stx_t
stx_dup (const stx_t src)
{
    Header* dst = dup(HEAD(src));

    return dst ? dst->data : NULL;
}


/*
Sets data length to zero.
*/
void 
stx_reset (stx_t s)
{
    Header* head = HEAD(s);

    if (!check(head)) 
        return;

    *FIELD_ADR(s,len) = 0;
    *s = 0;
}


/*
Change capacity.  
* If increased, the passed **reference** may get transparently updated.
* If lowered below length, data gets truncated.  
*/
bool
stx_resize (stx_t *pstx, const size_t newcap)
{
    Header* head = HEAD(*pstx);

    if (!check(head)) 
        return false;

    bool resized = resize(&head, newcap);

    *pstx = head->data;

    return resized;
}

/*
Appends at most `n` bytes from `src` to `dst`.  
* **No reallocation**.
* if `n` is zero, `strlen(src)` is used.
* Nothing done if input exceeds remaining space.

Return code :  
* `rc >= 0`  on success, as change in length.  
* `rc < 0`   on potential truncation, as needed capacity.
* `rc = 0`   on error.  

Ex : 
    buf = stx_new(5);
    stx_append_count (buf, "abc") =>  3
    stx_append_count (buf, "def") => -6   (needs capacity = 6)
*/
int 
stx_append_count (const stx_t dst, const char* src, const size_t n) 
{
    Header* head = HEAD(dst);

    if (!check(head)) 
        return STX_FAIL;

    return append_count (head, src, n);            
}


/*
Append `n` bytes of `src` to `*pdst`.

* If n is zero, `strlen(src)` is used.
* If over capacity, `*pdst` gets reallocated.

Return code :  
* `rc = 0`   on error.  
* `rc >= 0`  on success, as change in length. 
*/
size_t 
stx_append_count_alloc (stx_t *pdst, const char* src, const size_t n)
{
    Header* head = HEAD(*pdst);

    if (!check(head)) 
        return STX_FAIL;

    Header* saved = head;

    size_t rc = append_count_alloc (&head, src, n);
    
    if (head != saved) {
        LOG ("ptr changed");
        *pdst = head->data;
    }

    return rc;
}


/*
Appends a formatted c-string to `dst`, in place.  

* **No reallocation**.
* Nothing done if input exceeds remaining space.

Return code :  
* `rc >= 0`  on success, as increase in length.  
* `rc < 0`   on potential truncation, as needed capacity.  
* `rc = 0`   on error.  

Ex:
    stx_append_format (foo, "%s has %d apples", "Mary", 10);
*/
int 
stx_append_format (const stx_t dst, const char* fmt, ...) 
{
    Header* head = HEAD(dst);

    if (!check(head)) 
        return STX_FAIL;

    va_list args;
    va_start(args, fmt);
    int rc = append_format (head, fmt, args);            
    va_end(args);

    return rc;
}


/*
Compares `a` and `b`'s data string.  
* Capacities are not compared.
* Faster than `memcmp` since stored lengths are compared first.
*/
bool 
stx_equal (const stx_t a, const stx_t b) 
{
    const Header* ha = HEAD(a);
    const Header* hb = HEAD(b);

    return \
            check(ha) && check(hb) 
        &&  (ha->len == hb->len)
        &&  !memcmp (ha->data, hb->data, ha->len);
}


/*
Current capacity accessor.
*/
size_t 
stx_cap (const stx_t s)
{
    return check(HEAD(s)) ? FIELD_VAL(s,cap) : 0;
}

/*
Current length accessor.
*/
size_t 
stx_len (const stx_t s)
{
    return check(HEAD(s)) ? FIELD_VAL(s,len) : 0;
}

/*
Remaining space.
*/
size_t 
stx_spc (const stx_t s)
{
    return check(HEAD(s)) ? (FIELD_VAL(s,cap) - FIELD_VAL(s,len)) : 0;
}

/*
Check if *s* has a valid header.  
If it has a valid *cookie* and meaningful fields, then it's a fine duck.
*/
bool
stx_check (const stx_t s)
{
    return check(HEAD(s));
}

/*
Utility. Prints the state of `s`.
*/
void 
stx_show (const stx_t s)
{
    const Header* head = HEAD(s);

    if (!check(head)) {
        ERR ("stx_show: invalid header\n");
        return;
    }

    printf ("cap:%zu len:%zu data:'%s'\n",
        (size_t)head->cap, (size_t)head->len, head->data);

    fflush(stdout);
}