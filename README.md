![logo](assets/logo2.png)

# Stricks
Experimental managed C-strings library.  
version 0.2.0
:orange_book: [API](#API)

![schema](assets/block.png)

## Why ?

To make C-strings easier, safer and faster.  

Normal `char*` strings are a painful and risky business.  
Appending while keeping track of length, ensure null-termination,  
realloc, dangling pointers, etc...  

Plus, despite the low-level feel of manual handling, the ex-or-implicit calls  
to `strlen` may be a speed concern.  

Say you're making a forum engine, where a *page* is a fixed-size buffer.  
How to safely feed a page with posts without truncation ?


### The hard way

```C
char page[PAGE_SZ];

// Keep track
size_t page_len = 0;

while(1) {

    char* user = db("user");
    char* text = db("text");

    // Will the null be counted ?
    // Need to lookup `snprintf`...
    // In fact it won't !
    int post_len = snprintf (
        // Keep track
        page + page_len,
        // Will it be null-terminated ?
        // Need to lookup `snprintf` again...
        // In fact it will !
        PAGE_SZ - page_len, 
        "<div>%s<br>%s</div>", user, text
    );
    
    if (page_len + post_len + 1 > PAGE_SZ) {
        // Does not fit. Undo.
        page[page_len] = 0;
        break;
    }

    page_len += post_len;
}
```

### The stricky way

```C
stx_t page = stx_new(PAGE_SZ);

while(1) {

    char* user = db("user");
    char* text = db("text");

    if (stx_catf(page, "<div>%s<br>%s</div>", user, text) <= 0)
        break;
}
```

### Quick sample

The *example* folder implements the 'forum'.  
It reads rows from a mock db, and renders them to a page of limited size.  
When the next post won't fit, we flush the page and start anew.  

`make && cd example && ./forum`


# Principle

The `stx_t` (or "*strick*") type is just a normal `char*` string.  

```C
typedef char* stx_t;
```

The trick :wink: lies **before** the *stx_t* address :  

```C
Header {   
         cap;  
         len;  
         cookie; 
         flags;
    char data[];
}
```

*Header* holds the string's meta-data, taking care of state and bounds-checking.  
The `stx_t` type points directly to the `data` member.  

```
-------------------------------------------------------
| cap | len | cookie | flags | data                   |
-------------------------------------------------------
                             ^
                             |
            stx_t ------------
```

Header and data occupy a **single block** of memory (an "*SBlock*"),  
avoiding a further indirection like you find in typical `{len,*str}` schemes.    

The *SBlock* is of no concern to the user,  
who only passes and gets `stx_t` from the API.    
Of course, being really a `char*`, a *strick* can be passed to any  
(non-modifying) `string.h` functions.  

The above pseudo-code is simplified. In reality, Stricks defines several header types to optimize space for short strings, and houses the *cookie* and *flags* attributes in a separate `struct`.

```C
stx_t s = stx_from("Stricks");

stx_cata(s, " rule!");        
printf("%s\n", s);

//> Stricks rule!

```

This technique is used in Microsoft [BSTR](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/automat/bstr) and antirez [SDS](https://github.com/antirez/sds).  


# Security

No memory fault should be possible through the Stricks API.  
All methods check for a valid *Header*.  
If not found, no action is taken and a *falsy* value gets returned.  
(See *[stx_free](#stx_free)*)




# API

[stx_new](#stx_new)  
[stx_from](#stx_from)  
[stx_dup](#stx_dup)  

[stx_free](#stx_free)  
[stx_reset](#stx_reset)  
[stx_resize](#stx_resize)  
[stx_equal](#stx_equal)  
[stx_check](#stx_check)  
[stx_show](#stx_show)  

[stx_append](#stx_append) / stx_cat  
[stx_append_alloc](#stx_append_alloc) / stx_cata  
[stx_append_count](#stx_append_count) / stx_ncat  
[stx_append_format](#stx_append_format) / stx_catf  
[stx_append_count_alloc](#stx_append_count_alloc) / stx_ncata  

[stx_cap](#stx_cap)  
[stx_len](#stx_len)  
[stx_spc](#stx_spc)  



Custom allocator and destructor can be defined with  
```C
#define STX_MALLOC ...
#define STX_FREE ...
```

### stx_new
```C
stx_t stx_new (const size_t cap)
```
Allocates and inits a new `strick` of capacity `cap`.  

### stx_from
```C
stx_t stx_from (const char* src)
```
Creates a new strick and copies `src` to it.  
Capacity gets trimmed down to length.
```C
stx_t s = stx_from("Stricks");
stx_show(s); 
// cap:7 len:7 data:'Stricks'

```

### stx_dup
```C
stx_t stx_dup (const stx_t src)
```
Creates a duplicate strick of `src`.  
Capacity gets trimmed down to length.
```C
stx_t s = stx_new(16);
stx_cat(s, "foo");
stx_t dup = stx_dup(s);
stx_show(dup); 
// cap:3 len:3 data:'foo'

```

### stx_free
```C
void stx_free (stx_t s)
```
Releases the underlying SBlock.  

:cake: **Security** : Once the SBlock is freed, **no use-after-free or double-free**  should be possible through the Strick API :  

```C
stx_t s = stx_new(16);
stx_append(s, "foo");
stx_free(s);
```

```C
// Use-after-free
stx_append(s, "bar");
```
Nothing is done. Returns `STX_FAIL`.  


```C
// Double-free
stx_free(s);
```
Nothing is done.

#### How it works :wrench:
On first call, `stx_free(s)` zeroes-out the header, erasing the `cookie` canary.  
All subsequent API calls check for the canary, find it dead, then do nothing.

### stx_reset    
```C
void stx_reset (const stx_t s)
```
Sets data length to zero.  
```C
stx_t s = stx_new(16);
stx_cat(s, "foo");
stx_reset(s);
stx_show(s); 
// cap:16 len:0 data:''
```

### stx_resize    
```C
bool stx_resize (stx_t *pstx, const size_t newcap)
```
Change capacity.  
* If increased, the passed **reference** may get transparently updated.
* If lowered below length, data gets truncated.  

Returns: `true`/`false` on success/failure.
```C
stx_t s = stx_new(3);
int rc = stx_cat(s, "foobar"); // -> -6
if (rc<0) stx_resize(&s, -rc);
stx_cat(s, "foobar");
stx_show(s); 
// cap:6 len:6 data:'foobar'
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
size_t stx_ncata (stx_t *pdst, const char* src, const size_t n)
```
Append `n` bytes of `src` to `*pdst`.

* If n is zero, `strlen(src)` is used.
* If over capacity, `*pdst` gets reallocated.

Return code :  
* `rc = 0`   on error.  
* `rc >= 0`  on success, as change in length. 


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


### stx_check    
```C
bool stx_check (const stx_t s)
```
Check if *s* has a valid header.  

### stx_cap  
```C
size_t stx_cap (const stx_t s)
```
Current capacity accessor.

### stx_len  
```C
size_t stx_len (const stx_t s)
```
Current length accessor.

### stx_spc  
```C
size_t stx_spc (const stx_t s)
```
Remaining space.


### stx_equal    
```C
bool stx_equal (const stx_t a, const stx_t b)
```
Compares `a` and `b`'s data string.  
* Capacities are not compared.
* Faster than `memcmp` since stored lengths are compared first.


### stx_show    
```C
void stx_show (const stx_t s)
```
Utility. Prints the state of `s`.
```C
stx_show(foo);
// cap:8 len:5 data:'hello'
```



## Usage

```C
// app.c

#include <stdio.h>
#include "stx.h"

int main() {

    char name[] = "Alco";
    stx_t msg = stx_new(100);

    stx_catf(msg, "Hello! My name is %s.", name);
    puts(msg);

    return 0;
}
```

```
$ gcc app.c libstx -o app && ./app
Hello! My name is Alco.
```

## Build & unit-test

`make && make check`

## TODO
* utf8
* Slices / StringView
* High-level methods like *split()* and stuff.