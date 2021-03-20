/*
Stricks v0.2.3
Copyright (C) 2021 - Francois Alcover <francois[@]alcover.fr>
NO WARRANTY EXPRESSED OR IMPLIED.
*/

#ifndef STRICKS_H
#define STRICKS_H

#include <string.h>
#include <stdbool.h>

#ifndef STX_MALLOC
#define STX_MALLOC malloc
#endif

#ifndef STX_CALLOC
#define STX_CALLOC calloc
#endif

#ifndef STX_REALLOC
#define STX_REALLOC realloc
#endif

#ifndef STX_FREE
#define STX_FREE free
#endif

// Print warnings : double-free, truncation, etc..
// #define STX_WARNINGS

#define STX_MIN_CAP 2

typedef char* stx_t;

const stx_t stx_new (const size_t cap);
const stx_t stx_from (const char* src);
const stx_t stx_from_len (const char* src, const size_t len);

const stx_t stx_dup (const stx_t src);

size_t stx_cap (const stx_t s); // capacity accessor
size_t stx_len (const stx_t s); // length accessor
size_t stx_spc (const stx_t s); // remaining space

void	stx_free (const stx_t s);
void	stx_reset (const stx_t s);
void	stx_update (const stx_t s);
void	stx_trim (const stx_t s);
void	stx_show (const stx_t s); 
bool	stx_resize (stx_t *pstx, const size_t newcap);
bool	stx_check (const stx_t s);
bool	stx_equal (const stx_t a, const stx_t b);
stx_t* 	stx_split (const void* s, size_t len, const char* sep, unsigned int* outcnt);

intmax_t stx_append (stx_t dst, const char* src);
intmax_t stx_append_count (stx_t dst, const char* src, const size_t n);
intmax_t stx_append_format (const stx_t dst, const char* fmt, ...);
size_t	 stx_append_alloc (stx_t* dst, const char* src);
size_t	 stx_append_count_alloc (stx_t* dst, const char* src, const size_t n);

// Shorthands
#define stx_cat		stx_append
#define stx_ncat	stx_append_count
#define stx_catf	stx_append_format
#define stx_cata	stx_append_alloc
#define stx_ncata	stx_append_count_alloc

#endif