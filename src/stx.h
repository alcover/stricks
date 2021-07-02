/*
Stricks - Managed C strings library
Copyright (C) 2021 - Francois Alcover <francois[@]alcover.fr>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef STRICKS_H
#define STRICKS_H

#include <string.h>

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

#ifdef STX_WARNINGS
	#define STX_WARN(args...) ERR(args)
#else
	#define STX_WARN(args...) ((void)0)
#endif

#define STX_LOCAL_MEM 1024
#define STX_POOL_MEM 32*1024*1024

typedef const char* stx_t;

// create
stx_t	stx_new (size_t cap);
stx_t	stx_from (const char* src);
stx_t	stx_from_len (const void* src, size_t srclen);
stx_t	stx_dup (stx_t src);
stx_t*	stx_split (const char* src, const char* sep, int* outcnt);
stx_t*	stx_split_len (const char* src, size_t srclen, const char* sep, size_t seplen, int* outcnt);
stx_t 	stx_join (stx_t *list, int count, const char* sep);
stx_t 	stx_join_len (stx_t *list, int count, const char* sep, size_t seplen);

// append
size_t		stx_append (stx_t* dst, const void* src, size_t srclen);
long long	stx_append_strict (stx_t dst, const void* src, size_t srclen);
size_t		stx_append_fmt (stx_t* dst, const char* fmt, ...);
long long	stx_append_fmt_strict (stx_t dst, const char* fmt, ...);

// adjust/dispose
void	stx_free (stx_t s);
void	stx_list_free (const stx_t* list);
int		stx_resize (stx_t *pstx, size_t newcap);
void	stx_reset (stx_t s);
void	stx_adjust (stx_t s);
void	stx_trim (stx_t s);

// assess
size_t	stx_cap (stx_t s); // capacity accessor
size_t	stx_len (stx_t s); // length accessor
size_t	stx_spc (stx_t s); // remaining space
int		stx_equal (stx_t a, stx_t b);
void 	stx_dbg (stx_t s);

// shorthands
#define stx_cat		stx_append
#define stx_cats	stx_append_strict
#define stx_catf	stx_append_fmt
#define stx_catfs	stx_append_fmt_strict


#endif