/*
Stricks - Managed C strings library
Copyright (C) 2021 - Francois Alcover <francois[@]alcover.fr>
NO WARRANTY EXPRESSED OR IMPLIED
*/

#ifndef ALCO_UTIL_H
#define ALCO_UTIL_H

#define min(a,b) ({ \
__typeof__ (a) _a = (a); \
__typeof__ (b) _b = (b); \
_a > _b ? _b : _a; })

#define max(a,b) ({ \
__typeof__ (a) _a = (a); \
__typeof__ (b) _b = (b); \
_a > _b ? _a : _b; })


#define GETBIT(n,i) (((n) >> (i)) & 1)
#define SETBIT(n,i) (n) |= (1 << (i))


// draft
static char*
str_repeat (const char* pat, int n)
{
    const size_t patlen = strlen(pat);
    const size_t retlen = n*patlen;
    char* ret = malloc(retlen+1);
    
    *ret = 0;
    ret[retlen] = 0;
    for(int i=0;i<n;++i) memcpy(ret+i*patlen, pat, patlen);

    return ret;
}

static const char*
str_cat (const char* a, const char* b)
{
    if(!a) return b;
    if(!b) return a;

    const size_t len = strlen(a) + strlen(b);
    char* ret = malloc(len+1);
    ret[0] = 0;
    ret[len] = 0;
    strcat(ret,a);
    strcat(ret,b);

    return ret;
}


static char*
load (const char* src_path, size_t* outlen)
{
    FILE *file = fopen(src_path, "rb");

    if (!file) {
        perror("fopen");
        *outlen = 0;
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    size_t len = ftell(file);
    char* out = malloc(len+1);
    
    rewind(file);
    fread ((char*)out, len, 1, file);
    out[len] = 0;
    fclose(file);
    
    *outlen = len;
    
    return out;
}

#endif