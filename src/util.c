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
	    end = beg;
        }
    }

    callback (str + beg, end - beg, ctx);
}

// alco - unverified
static inline int
str_count_str (const char *str, const char* tok)
{
	const size_t toklen = strlen(tok);
	int ret = 0;
	for (const char* tmp = str; (tmp = strstr(tmp,tok)); tmp += toklen, ++ret);
	return ret;
}
