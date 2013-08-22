/* General-use routines */

#include <stdlib.h>
#include <stdio.h>
#include "tree.h"

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

token_t *findtok(exp_tree_t *et)
{
	int i;
	token_t *t;
	if (et->tok)
		return et->tok;
	for (i = 0; i < et->child_count; ++i)
		if ((t = findtok(et->child[i])))
			return t;
	return NULL;
}
