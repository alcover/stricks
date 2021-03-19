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


static inline void 
str_split (const char *str, const char* sep, 
	void(*callback)(const char* tok, size_t len, void* ctx), 
	void *ctx)
{
	if (!str||!sep) return;

    const size_t seplen = strlen(sep);

    if (!seplen) {
    	callback(str, strlen(str), ctx); // ?
    	return;
    }

    const char *beg = str, *end = str;

    while (*end) {
        if (!strncmp (end, sep, seplen)) {
            callback (beg, end - beg, ctx);
            end += seplen;
            beg = end;
        } else {
        	++end;
        }
    }

    callback (beg, end - beg, ctx); // last part
}

static char*
rand_str (size_t len)
{
    const char charset[] = "abcde";

    char* out = malloc(len+1);
    if (!out) return out;
    
    char* p=out;
    while (len--)
        *p++ = charset[rand() % (int) (sizeof(charset)-1)];
    *p = 0;
    
    return out;
}