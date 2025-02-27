#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

#include "cheri_helpers.h"

#define ALIGN (sizeof(size_t)-1)
#define ONES ((size_t)-1/UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX/2+1))
#define HASZERO(x) ((x)-ONES & ~(x) & HIGHS)
#define MIN(a,b) ((a)<(b)?(a):(b))

void *memccpy(void *restrict dest, const void *restrict src, int c, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	size_t i = 0;
	size_t max_i = MIN(CAP_TAIL_LENGTH(dest), CAP_TAIL_LENGTH(src));

	c = (unsigned char)c;
#ifdef __GNUC__
	typedef size_t __attribute__((__may_alias__)) word;
	word *wd;
	const word *ws;
	if (((uintptr_t)s & ALIGN) == ((uintptr_t)d & ALIGN)) {
		for (; ((uintptr_t)s & ALIGN) && n && (*d=*s)!=c; n--, s++, d++, i++);
		if ((uintptr_t)s & ALIGN) goto tail;
		if (LT_IF_CHERI_ELSE(i + sizeof(word) - 1, max_i, false)) {
			size_t k = ONES * c;
			wd = (void *)d;
			ws = (const void *)s;
			for (;
			     LT_IF_CHERI_ELSE(i + sizeof(word) - 1, max_i, true) &&
			     n >= sizeof(size_t) && !HASZERO(*ws ^ k);
			     n -= sizeof(size_t), ws++, wd++, i += sizeof(word))
				*wd = *ws;
			d = (void *)wd;
			s = (const void *)ws;
		}
	}
#endif
	for (; n && (*d=*s)!=c; n--, s++, d++);
tail:
	if (n)
	  return d+1;
	return 0;
}
