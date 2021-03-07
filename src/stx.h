/*
Stricks v0.2.0
Copyright (C) 2021 - Francois Alcover <francois@alcover.fr>
NO WARRANTY EXPRESSED OR IMPLIED.
*/

#ifndef STRICKS_H
#define STRICKS_H

#include <string.h>
#include <stdbool.h>

// Uncomment to display warnings (double free, truncation,...)
// #define STX_WARNINGS

#ifndef STX_MALLOC
#define STX_MALLOC malloc
#endif

#ifndef STX_FREE
#define STX_FREE free
#endif

#define STX_FAIL 0

typedef char* stx_t;

const stx_t	stx_new (const size_t cap);
const stx_t	stx_from (const char* src);
const stx_t stx_dup (const stx_t src);
void stx_free (const stx_t s);
void stx_reset (const stx_t s);
void stx_show (const stx_t s); 
bool stx_resize (stx_t *pstx, const size_t newcap);
bool stx_check (const stx_t s);
bool stx_equal (const stx_t a, const stx_t b);

size_t stx_cap (const stx_t s); // Capacity accessor
size_t stx_len (const stx_t s); // Length accessor
size_t stx_spc (const stx_t s); // Remaining space

#define stx_append(dst, src) stx_append_count (dst, src, 0)
#define stx_append_alloc(pdst, src) stx_append_count_alloc (pdst, src, 0)
int		stx_append_format (const stx_t dst, const char* fmt, ...);
int		stx_append_count (const stx_t dst, const char* src, const size_t n);
size_t 	stx_append_count_alloc (stx_t *pdst, const char* src, const size_t n);

// Shorthands
#define stx_cat		stx_append
#define stx_cata	stx_append_alloc
#define stx_catf	stx_append_format
#define stx_ncat	stx_append_count
#define stx_ncata	stx_append_count_alloc

#endif