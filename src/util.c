#define ASSERT(c, m) { \
if (!(c)) { \
    fprintf(stderr, __FILE__ ":%d: assertion %s failed: %s\n", __LINE__, #c, m); \
    exit(1); \
} \
}

static inline size_t
strnlen (const char *s, size_t maxlen)
{
	size_t len;

	for (len = 0; len < maxlen; ++len)
		if (!s[len]) break;

	return len;
}



// alco - unverified
static inline void 
str_split (const char *str, const char* sep, 
	void(*callback)(const char* tok, size_t len, void* ctx), 
	void *ctx)
{
    const size_t seplen = strlen(sep);
    unsigned int beg = 0, end = 0;

    for (; str[end]; end++) {
        if (!strncmp(str+end, sep, seplen)) {
            callback (str + beg, end - beg, ctx);
            beg = end + seplen;
        }
    }

    callback (str + beg, end - beg, ctx);
}

static inline int
str_count_str (const char *str, const char* tok)
{
	int ret = 0;
	for (const char* tmp = str; (tmp = strstr(tmp,tok)); ++tmp, ++ret);
	return ret;
}