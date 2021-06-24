![logo](assets/logo.png)

# Stricks
Augmented strings for C  
 
![CI](https://github.com/alcover/stricks/actions/workflows/ci.yml/badge.svg)  

:orange_book: [API](#API)

*Stricks* make C strings easy and fast.  
Managing bounds, storage and reallocations is done for you,  
without resorting to a dedicated datatype.

# Principle

```C
typedef const char* stx_t;
```  

A *strick* is just a normal `char*`, and can be passed to `<string.h>` functions.  
Its metadata, invisible to the user, are set in a prefix header :    

![schema](assets/schema.png)

Header and string occupy a **continuous** block of memory,  
avoiding the further indirection of `{len,*str}` schemes.    
This technique is notably used in [SDS](https://github.com/antirez/sds), 
now part of [Redis](https://github.com/redis/redis).

## Usage


```C
#include <stdio.h>
#include "stx.h"

int main() {
    stx_t s = stx_from("Stricks");
    stx_append(&s, " or treats!");        
    printf(s);
    return 0;
}
```

```
$ gcc app.c stx -o app && ./app
Stricks or treats!
```

#### Sample

*src/example.c* implements a mock forum with fixed size pages.  
When the next post would truncate, the buffer is flushed.  

`make && ./bin/example`


## Build & unit-test

`make && make check`

## Speed

`make && make bench`  

*Stricks* are sometimes much faster than [SDS](https://github.com/antirez/sds).  

On Intel Core i3 :
```
init and free
---------------
8 bytes strings :
      SDS  551 ms
  Stricks  436 ms
256 bytes strings :
      SDS  822 ms
  Stricks  795 ms

append
---------------
8 bytes strings :
      SDS  679 ms
  Stricks  447 ms
256 bytes strings :
      SDS  449 ms
  Stricks  172 ms

split and join
---------------
8 bytes strings :
      SDS  577 ms
  Stricks  227 ms
256 bytes strings :
      SDS  700 ms
  Stricks  79 ms
```







# API

#### create
[stx_new](#stx_new)  
[stx_from](#stx_from)  
[stx_from_len](#stx_from_len)  
[stx_dup](#stx_dup)  
[stx_split](#stx_split)  
[stx_join](#stx_join)  
[stx_join_len](#stx_join_len)

#### append
[stx_append](#stx_append)  
[stx_append_strict](#stx_append_strict)  
[stx_append_fmt](#stx_append_fmt)  
[stx_append_fmt_strict](#stx_append_fmt_strict)  

#### free / adjust
[stx_free](#stx_free)  
[stx_list_free](#stx_list_free)  
[stx_reset](#stx_reset)  
[stx_resize](#stx_resize)  
[stx_adjust](#stx_adjust)  
[stx_trim](#stx_trim)  

#### assess
[stx_cap](#stx_cap)  
[stx_len](#stx_len)  
[stx_spc](#stx_spc)  
[stx_equal](#stx_equal)  
[stx_dbg](#stx_dbg)  


Custom allocators can be defined with  
```
#define STX_MALLOC  my_malloc
#define STX_REALLOC my_realloc
#define STX_FREE    my_free
```

### stx_new
Create a *strick* of capacity `cap`.  
```C
stx_t stx_new (size_t cap)
```

```C
stx_t s = stx_new(10);
stx_dbg(s); 
//cap:10 len:0 data:""
```

### stx_from
Create a *strick* by copying string `src`.  
```C
stx_t stx_from (const char* src)
```

```C
stx_t s = stx_from("Bonjour");
stx_dbg(s); 
// cap:7 len:7 data:'Bonjour'

```

### stx_from_len

Create a *strick* by copying `srclen` bytes from `src`.  

```C
stx_t stx_from_len (const void* src, size_t srclen)
```

This function is **binary** : exactly `srclen` bytes will be copied, *NUL*s included.  
If you need the `stx.len` property to reflect its C-string length, use `stx_adjust` on the result.

```C
stx_t s = stx_from_len("Hello!", 4);
printf(s); // 'Hell'
```

### stx_dup
Create a duplicate *strick* of `src`.  
```C
stx_t stx_dup (stx_t src)
```
Capacity is adjusted to length.

```C
stx_t s = stx_new(16);
stx_append(&s, "foo");
stx_t dup = stx_dup(s);
stx_dbg(dup); 
// cap:3 len:3 data:'foo'

```


### stx_split
Split `src` on separator `sep` into an array of *stricks* of resulting length `*outcnt`. 
```C
stx_t*  
stx_split (const char* src, const char* sep, int* outcnt);
```

```C
int cnt = 0;
stx_t* list = stx_split("foo|bar", "|", &cnt);

for (int i = 0; i < cnt; ++i) {
    stx_dbg(list[i]);
}

// cap:3 len:3 data:'foo'
// cap:3 len:3 data:'bar'

```
Or using the list sentinel :

```C
while (part = *list++) {
    stx_dbg(part);
}
```

### stx_split_len
Core split method with arbitrary lengths. 
```C
stx_t*  
stx_split_len (const char* src, size_t srclen, const char* sep, size_t seplen, int* outcnt)
```


### stx_join
Join the *stricks* `list` of length *count* using separator *sep* into a new *strick*.
```C
stx_t stx_join (stx_t *list, int count, const char* sep);
```
```C
int count = 0;
stx_t* parts = stx_split ("foo|bar", "|", &count);
stx_t joined = stx_join (parts, count, "|");
printf(joined); // "foo|bar"
```

### stx_join_len
Same with known separator length.
```C
stx_t stx_join_len (stx_t *list, int count, const char* sep, size_t seplen);
```


### stx_append
stx_cat
  
```C
size_t stx_append (stx_t* dst, const void* src, size_t srclen)
```
Appends `len` bytes from `src` to `*dst`.

* If over capacity, `*dst` gets **reallocated**.
* reallocation reserves 2x the needed memory.

Return code :  
* `rc = 0`   on error.  
* `rc >= 0`  on success, as new length.  

```C
stx_t s = stx_from("abc"); 
stx_append(&s, "def"); //-> 6 
stx_dbg(s); 
// cap:12 len:6 data:'abcdef'
```


### stx_append_strict
stx_cats
 
```C
long long stx_append_strict (stx_t dst, const char* src, size_t srclen)
```
Appends `len` bytes from `src` to `dst`, in place.  
* **No reallocation**.
* Nothing done if input exceeds remaining space.

Return code :  
* `rc >= 0`  on success, as new length.  
* `rc < 0`   on potential truncation, as needed capacity. 
* `rc = 0`   on error.  

```C
stx_t s = stx_new(5);  
stx_append_strict(s, "abc"); //-> 3
printf(s); // "abc"  
stx_append_strict(s, "def"); //-> -6  (would need capacity = 6)
printf(s); // "abc"
```


### stx_append_fmt   
stx_catf
```C
size_t stx_append_fmt (stx_t* dst, const char* fmt, ...)
```
Appends a formatted string to `*dst`.  

* If over capacity, `*dst` gets **reallocated**.
* reallocation reserves 2x the needed memory.

Return code :  
* `rc = 0`   on error.  
* `rc >= 0`  on success, as new length.  

```C
stx_t foo = stx_new(32);
stx_append_fmt (&foo, "%s has %d apples", "Mary", 10);
stx_dbg(foo);
// cap:32 len:18 data:'Mary has 10 apples'
```


### stx_append_fmt_strict   
stx_catfs
```C
long long stx_append_fmt_strict (stx_t dst, const char* fmt, ...)
```
Appends a formatted string to `dst`, in place.  

* **No reallocation**.
* Nothing done if input exceeds remaining space.

Return code :  
* `rc >= 0`  on success, as new length.  
* `rc < 0`   on potential truncation, as needed capacity.  
* `rc = 0`   on error.  

```C
stx_t foo = stx_new(32);
stx_append_fmt (foo, "%s has %d apples", "Mary", 10);
stx_dbg(foo);
// cap:32 len:18 data:'Mary has 10 apples'
```





### stx_free
Releases the enclosing memory block.  
```C
void stx_free (stx_t s)
```

```C
stx_t s = stx_from("foo");
stx_free(s);
```

### stx_list_free
Releases a list of parts generated by *stx_split*.
```C
void stx_list_free (const stx_t* list)
```
```C
int cnt = 0;
stx_t* list = stx_split("foo|bar", "|", &cnt);
// (..use list..)
stx_list_free(list);
```

### stx_reset    
Sets length to zero.  
```C
void stx_reset (stx_t s)
```
```C
stx_t s = stx_new(16);
stx_append_strict(s, "foo");
stx_reset(s);
stx_dbg(s); 
// cap:16 len:0 data:''
```

### stx_resize    
Change capacity.  
```C
int stx_resize (stx_t *pstx, size_t newcap)
```
* If increased, the passed **reference** may get updated.
* If lowered below length, data gets truncated.  

Returns: `true/false` on success/failure.
```C
stx_t s = stx_new(3);
int rc = stx_append_strict(s, "foobar"); // -> -6
if (rc<0) stx_resize(&s, -rc);
stx_append_strict(s, "foobar");
stx_dbg(s); 
// cap:6 len:6 data:'foobar'
```

### stx_adjust
Sets `len` straight in case data was modified from outside.
```C
void stx_adjust (stx_t s)
```

```C
stx_t s = stx_from("foobar");

// out-of-API modification
((char*)s)[3] = '\0';
stx_dbg(s);
// cap:6 len:6 data:'foo'  WRONG LEN!

stx_adjust(s);
stx_dbg(s);
// cap:6 len:3 data:'foo'  FAITHFUL
```


### stx_trim
Removes white space, left and right, preserving capacity.
```C
void stx_trim (stx_t s)
```

```C
stx_t s = stx_from(" foo ");
stx_trim(s);
stx_dbg(s);
// cap:5 len:3 data:'foo'
```



### stx_cap  
Current capacity accessor.  
`size_t stx_cap (stx_t s)`

### stx_len  
Current length accessor.  
`size_t stx_len (stx_t s)`

### stx_spc  
Remaining space.  
`size_t stx_spc (stx_t s)`

### stx_equal    
Compares `a` and `b`'s data string.  
```C
int stx_equal (stx_t a, stx_t b)
```
* Capacities are not compared.
* Faster than `memcmp` since stored lengths are compared first.

### stx_dbg    
Printing the state of a *strick*.  
```C
stx_t s = stx_from("foo");
stx_dbg(foo);
// cap:3 len:3 data:'foo'
```