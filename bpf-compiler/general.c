/* General-use routines */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

int numstr(char *str)
{
	char *p = str;
	for (; *p; ++p)
		if (!isdigit(*p))
			return 0;
	return *str != 0;
}

void fail(char* mesg)
{
	fprintf(stderr, "error: %s\n", mesg);
	exit(1);
}

void sanity_requires(int exp)
{
	if(!exp)
		fail("that doesn't make any sense");
}
