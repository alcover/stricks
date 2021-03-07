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