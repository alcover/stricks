static inline size_t
strnlen (const char *s, size_t maxlen)
{
	size_t len;

	for (len = 0; len < maxlen; ++len)
		if (!s[len]) break;

	return len;
}