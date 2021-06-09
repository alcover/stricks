#ifndef ALCO_UTIL_H
#define ALCO_UTIL_H

#define GETBIT(n,i) (((n) >> (i)) & 1)
#define SETBIT(n,i) (n) |= (1 << (i))

static inline size_t
str_count (const char *str, const char* tok/*, size_t* outlen*/)
{
    if (!str||!tok) return 0;

    const size_t toklen = strlen(tok);
    if (!toklen) return 0;
    
    size_t cnt = 0;
    const char* s = str; 
    
    while ((s = strstr(s,tok))) {
        ++cnt;
        s += toklen;
    }

    return cnt;
}


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