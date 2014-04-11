/* Some assorted general-use helper routines */

#include <stdlib.h>
#include <stdio.h>
#include "tree.h"

void* my_strdup(char *s)
{
	char *n = malloc(strlen(s) + 1);
	strcpy(n, s);
	return n;
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

/*
 * Try to find a token pointer somewhere
 * in a tree. This is used to trace errors
 * in the parse trees to their originating tokens,
 * and from there to the originating line,
 * so that the fancy error messages showing
 * line number etc. can be printed.
 */
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
