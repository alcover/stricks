![logo](assets/logo.png)

# Stricks
Managed C strings library.  
 
![CI](https://github.com/alcover/stricks/actions/workflows/ci.yml/badge.svg)  
:orange_book: [API](#API)

## Why ?

Because handling C strings is tedious and error-prone.  
Keeping track of length, null-byte, reallocation, etc...  
Speed is also a concern with excessive calls to `strlen()`.  


# Principle

A prefix header manages the string's state and bounds.  
The user-facing type `stx_t` points directly to the `data` member.  

![schema](assets/schema.png)

The `stx_t` type (or *strick*) is just a normal `char*` string.  

```C
typedef const char* stx_t;
```

Header and string occupy a **single block** of memory (an "*SBlock*"),  
avoiding the indirection you find in `{len,*str}` schemes.    

This technique is used notably by antirez/[SDS](https://github.com/antirez/sds).  

The *SBlock* is invisible to the user, who only uses `stx_t`.    
Being really a `char*`, *stricks* can be passed to any (non-modifying) `<string.h>` functions.  

The above illustration is simplified. In reality, Stricks uses two header types to optimize space, and houses the *canary* and *flags* in a separate *struct*. (See *src/stx.c*)



# Security

Stricks aims at limiting memory faults through the API :  

* typedef `const char*` forces the user to cast when she wants to write.
* all methods check for a valid *Header*.  
* if invalid, no action is taken and a *falsy* value is returned.  

(See *[stx_free](#stx_free)*)





## Usage


```C
// app.c
#include <stdio.h>
#include "stx.h"

int main() {

    stx_t s = stx_from("Stricks");
    stx_append_alloc(&s, " are treats!");        
    printf(s);

    return 0;
}
```

```
$ gcc app.c libstx -o app && ./app
Stricks are treats!
```

#### Sample

*src/example.c* implements a mock forum with fixed size pages.  
When the next post would truncate, the buffer is flushed.  

`make && ./bin/example`


## Build & unit-test

`make && make check`




# API

[stx_new](#stx_new)  
[stx_from](#stx_from)  
[stx_from_len](#stx_from_len)  
[stx_dup](#stx_dup)  

[stx_cap](#stx_cap)  
[stx_len](#stx_len)  
[stx_spc](#stx_spc)  

[stx_free](#stx_free)  
[stx_reset](#stx_reset)  
[stx_update](#stx_update)  
[stx_trim](#stx_trim)  
[stx_show](#stx_show)  
[stx_resize](#stx_resize)  
[stx_check](#stx_check)  
[stx_equal](#stx_equal)  
[stx_split](#stx_split)  
[stx_list_free](#stx_list_free)  

[stx_append](#stx_append) / stx_cat  
[stx_append_count](#stx_append_count) / stx_ncat  
[stx_append_format](#stx_append_format) / stx_catf  
[stx_append_alloc](#stx_append_alloc) / stx_cata  
[stx_append_count_alloc](#stx_append_count_alloc) / stx_ncata  



Custom allocator and destructor can be defined with  
```
#define STX_MALLOC  my_allocator
#define STX_REALLOC my_realloc
#define STX_FREE    my_free
```

### stx_new
Allocates and inits a new *strick* of capacity `cap`.  
```C
stx_t stx_new (size_t cap)
```

### stx_from
Creates a new *strick* by copying string `src`.  
```C
stx_t stx_from (const char* src)
```
Capacity is adjusted to length.

```C
stx_t s = stx_from("Stricks");
stx_show(s); 
// cap:7 len:7 data:'Stricks'

```


### stx_from_len

Creates a new *strick* with at most `len` bytes from `src`.  

```C
stx_t stx_from_len (const char* src, size_t len)
```

```C
stx_t s = stx_from_len("Hello!", 4);
stx_show(s); 
// cap:4 len:4 data:'Hell'
```

If `len > strlen(src)`, the resulting capacity is `len`.  

```C
stx_t s = stx_from_len("Hello!", 10);
stx_show(s); 
// cap:10 len:6 data:'Hello!'
```

### stx_dup
Creates a duplicate strick of `src`.  
```C
stx_t stx_dup (stx_t src)
```
Capacity gets trimmed down to length.

```C
stx_t s = stx_new(16);
stx_cat(s, "foo");
stx_t dup = stx_dup(s);
stx_show(dup); 
// cap:3 len:3 data:'foo'

```

### stx_cap  
Current capacity accessor.
```C
size_t stx_cap (stx_t s)
```

### stx_len  
Current length accessor.
```C
size_t stx_len (stx_t s)
```

### stx_spc  
Remaining space.
```C
size_t stx_spc (stx_t s)
```



### stx_reset    
Sets length to zero.  
```C
void stx_reset (stx_t s)
```
```C
stx_t s = stx_new(16);
stx_cat(s, "foo");
stx_reset(s);
stx_show(s); 
// cap:16 len:0 data:''
```

### stx_free
Releases the enclosing SBlock.  
```C
void stx_free (stx_t s)
```

:cake: **Security** :  
Once the block is freed, no *use-after-free* or *double-free* should be possible through the Strick API :  

```C
stx_t s = stx_new(16);
stx_append(s, "foo");
stx_free(s);

// Use-after-free
stx_append(s, "bar");
// No action. Returns 0.  
printf("%zu\n", stx_len(s));
// 0

// Double-free
stx_free(s);
// No action.
```

:wrench: **How it works**  
On first call, `stx_free(s)` zeroes-out the header, erasing the `canary`.  
All subsequent API calls, seeing no canary, do nothing.


### stx_resize    
Change capacity.  
```C
bool stx_resize (stx_t *pstx, size_t newcap)
```
* If increased, the passed **reference** may get transparently updated.
* If lowered below length, data gets truncated.  

Returns: `true/false` on success/failure.
```C
stx_t s = stx_new(3);
int rc = stx_cat(s, "foobar"); // -> -6
if (rc<0) stx_resize(&s, -rc);
stx_cat(s, "foobar");
stx_show(s); 
// cap:6 len:6 data:'foobar'
```


### stx_update
Sets `len` straight in case data was modified from outside.
```C
void stx_update (stx_t s)
```

### stx_trim
Removes white space, left and right.
```C
void stx_trim (stx_t s)
```
Capacity remains the same.


### stx_split
Splits a *strick* or string on separator `sep` into an array of *stricks*. 
```C
stx_t*
stx_split (const void* s, const char* sep, unsigned int *outcnt)
```
`*outcnt` receives the resulting array length.

```C
stx_t s = stx_from("foo, bar");
unsigned cnt = 0;

stx_t* list = stx_split(s, ", ", &cnt);

for (int i = 0; i < cnt; ++i) {
    stx_show(list[i]);
}

// cap:3 len:3 data:'foo'
// cap:3 len:3 data:'bar'

```
Or comfortably using the list sentinel

```C
while (part = *list++) {
    stx_show(part);
}
```

### stx_list_free
Releases a list of parts generated by *stx_split*.
```C
void stx_list_free (const stx_t* list)
```


### stx_equal    
Compares `a` and `b`'s data string.  
```C
bool stx_equal (stx_t a, stx_t b)
```
* Capacities are not compared.
* Faster than `memcmp` since stored lengths are compared first.


### stx_show    
Utility printing the state of `s`.
```C
void stx_show (stx_t s)
```
```C
stx_show(foo);
// cap:8 len:5 data:'hello'
```


### stx_check    
Check if *s* has a valid header.  
```C
bool stx_check (stx_t s)
```


### stx_append
stx_cat  
```C
int stx_append (stx_t dst, const char* src)
```
Appends `src` to `dst`.  
* **No reallocation**.
* Nothing done if input exceeds remaining space.

Return code :  
* `rc >= 0`  on success, as change in length.  
* `rc < 0`   on potential truncation, as needed capacity. 
* `rc = 0`   on error.  

```C
stx_t s = stx_new(5);  
stx_cat(s, "abc"); //-> 3
printf("%s", s); // "abc"  
stx_cat(s, "def"); //-> -6  (needs capacity = 6)
printf("%s", s); // "abc"
```


### stx_append_count
stx_ncat  
```C
int stx_ncat (stx_t dst, const char* src, size_t n)
```
Appends at most `n` bytes from `src` to `dst`.  
* **No reallocation**.
* if `n` is zero, `strlen(src)` is used.
* Nothing done if input exceeds remaining space.

Return code :  
* `rc >= 0`  on success, as change in length.  
* `rc < 0`   on potential truncation, as needed capacity.
* `rc = 0`   on error.  

```C
stx_t s = stx_new(5);  
stx_ncat(s, "abc", 2); //-> 2
printf("%s", s); // "ab"
```


### stx_append_format
stx_catf     
```C
int stx_catf (stx_t dst, const char* fmt, ...)
```
Appends a formatted c-string to `dst`, in place.  

* **No reallocation**.
* Nothing done if input exceeds remaining space.

Return code :  
* `rc >= 0`  on success, as increase in length.  
* `rc < 0`   on potential truncation, as needed capacity.  
* `rc = 0`   on error.  

```C
stx_t foo = stx_new(32);
stx_catf (foo, "%s has %d apples", "Mary", 10);
stx_show(foo);
// cap:32 len:18 data:'Mary has 10 apples'
```


### stx_append_alloc
stx_cata     
```C
size_t stx_cata (stx_t *pdst, const char* src)
```
Appends `src` to `*pdst`.

* If over capacity, `*pdst` gets **reallocated**.
* reallocation reserves 2x the needed memory.

Return code :  
* `rc = 0`   on error.  
* `rc >= 0`  on success, as change in length.  

```C
stx_t s = stx_new(3);  
stx_cat(s, "abc"); 
stx_cata(s, "def"); //-> 3 
stx_show(s); // "cap:12 len:6 data:'abcdef'"
```


### stx_append_count_alloc
stx_ncata     
```C
size_t stx_ncata (stx_t *pdst, const char* src, size_t n)
```
Append `n` bytes of `src` to `*pdst`.

* If n is zero, `strlen(src)` is used.
* If over capacity, `*pdst` gets reallocated.

Return code :  
* `rc = 0`   on error.  
* `rc >= 0`  on success, as change in length. 