#ifndef ALCO_UTIL_H
#define ALCO_UTIL_H

#define max(a,b) ({ \
__typeof__ (a) _a = (a); \
__typeof__ (b) _b = (b); \
_a > _b ? _a : _b; })


static inline size_t
strnlen (const char *s, const size_t n)
{
	size_t len = 0;
	for (; len < n && s[len]; ++len);
	return len;
}


static inline size_t
str_count (const char *str, const char* tok)
{
    if (!str||!tok) return 0;

    const size_t toklen = strlen(tok);
    if (!toklen) return 0;
    
    size_t cnt = 0;
    
    while ((str = strstr(str,tok))) {
        ++cnt;
        str += toklen;
    }

    return cnt;
}


static char*
rand_str (size_t len, const char* charset)
{
    const int setlen = strlen(charset);
    char* out = malloc(len+1);    
    char* p=out;

    while (len--)
        *p++ = charset[rand() % (setlen-1)];
    
    *p = 0;   
    return out;
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