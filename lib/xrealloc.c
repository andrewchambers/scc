
#include <stdlib.h>
#include <cc.h>

void *
xrealloc(void *buff, register size_t size)
{
	register void *p = realloc(buff, size);

	if (!p)
		die("out of memory");
	return p;
}