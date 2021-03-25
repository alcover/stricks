/*
Stricks v0.3.1
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
#ifndef STX_WARNINGS
#define STX_WARNINGS 0
#endif

#define STX_MIN_CAP 2

typedef const char* stx_t;

size_t	stx_cap (stx_t s); // capacity accessor
size_t	stx_len (stx_t s); // length accessor
size_t	stx_spc (stx_t s); // remaining space

stx_t	stx_new (size_t cap);
stx_t	stx_from (const char* src);
stx_t	stx_from_len (const char* src, size_t len);
stx_t	stx_dup (stx_t src);
stx_t	stx_load (const char* src);

void	stx_free (stx_t s);
void	stx_reset (stx_t s);
void	stx_update (stx_t s);
void	stx_trim (stx_t s);
void	stx_show (stx_t s); 
bool	stx_resize (stx_t *pstx, size_t newcap);
bool	stx_check (stx_t s);
bool	stx_equal (stx_t a, stx_t b);
stx_t* 	stx_split (const void* s, size_t len, const char* sep, unsigned* outcnt);

int		stx_append (stx_t dst, const char* src);
int		stx_append_count (stx_t dst, const char* src, size_t n);
int		stx_append_format (stx_t dst, const char* fmt, ...);
size_t	stx_append_alloc (stx_t* dst, const char* src);
size_t	stx_append_count_alloc (stx_t* dst, const char* src, size_t n);

// Shorthands
#define stx_cat		stx_append
#define stx_ncat	stx_append_count
#define stx_catf	stx_append_format
#define stx_cata	stx_append_alloc
#define stx_ncata	stx_append_count_alloc

#endif