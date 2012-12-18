#include <stdlib.h>
#include <stdio.h>

/* General-use routines */

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
