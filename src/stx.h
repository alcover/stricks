/*
Stricks v0.4.0
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

// double-free, truncation, ...
#ifdef STX_WARNINGS
	#define STX_WARN(args...) ERR(args)
#else
	#define STX_WARN(args...) ((void)0)
#endif

#define STX_STACK_MAX 256
#define STX_POOL_MAX 4*1024*1024

typedef const char* stx_t;

// create
stx_t	stx_new (size_t cap);
stx_t	stx_from (const char* src);
stx_t	stx_from_len (const void* src, size_t srclen);
stx_t	stx_dup (stx_t src);

// append
size_t		stx_append (stx_t* dst, const void* src, size_t srclen);
long long	stx_append_strict (stx_t dst, const void* src, size_t srclen);
size_t		stx_append_fmt (stx_t* dst, const char* fmt, ...);
long long	stx_append_fmt_strict (stx_t dst, const char* fmt, ...);

// split
stx_t*	stx_split (const char* src, const char* sep, size_t* outcnt);
stx_t*	stx_split_len (const char* src, size_t srclen, const char* sep, size_t seplen, size_t* outcnt);
// stx_t*	stx_split_fast (const char* src, size_t srclen, const char* sep, size_t* outcnt);
void	stx_list_free (const stx_t* list);

// adjust/dispose
void	stx_free (stx_t s);
void	stx_reset (stx_t s);
void	stx_adjust (stx_t s);
void	stx_trim (stx_t s);
bool	stx_resize (stx_t *pstx, size_t newcap);

// assess
size_t	stx_cap (stx_t s); // capacity accessor
size_t	stx_len (stx_t s); // length accessor
size_t	stx_spc (stx_t s); // remaining space
bool	stx_equal (stx_t a, stx_t b);
bool	stx_check (stx_t s);
void 	stx_dbg (stx_t s);
void 	stx_dbg_all (stx_t s);

// Shorthands
#define stx_cat		stx_append
#define stx_cats	stx_append_strict
#define stx_catf	stx_append_fmt
#define stx_catfs	stx_append_fmt_strict


#endif