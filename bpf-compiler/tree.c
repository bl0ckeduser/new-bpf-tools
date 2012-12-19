#include "tree.h"
#include "tokenizer.h"
#include "tokens.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* FIXME: crazy mallocs and memcpys, no garbage collection */

extern void fail(char* mesg);

exp_tree_t *alloc_exptree(exp_tree_t et)
{
	exp_tree_t *p = malloc(sizeof(exp_tree_t));
	if (!p)
		fail("malloc et");
	*p = et;
	return p;
}

int valid_tree(exp_tree_t et)
{
	return et.head_type != NULL_TREE;
}

exp_tree_t new_exp_tree(unsigned int type, token_t* tok)
{
	token_t* tok_copy = NULL;

	if (tok) {
		tok_copy = malloc(sizeof(token_t));
		if (!tok_copy)
			fail("malloc tok_copy");
		memcpy(tok_copy, tok, sizeof(token_t));
	}

	exp_tree_t tr;
	tr.head_type = type;
	tr.tok = tok_copy;
	tr.child_count = 0;
	tr.child_alloc = 64;
	tr.child = malloc(64 * sizeof(exp_tree_t *));
	if (!tr.child)
		fail("malloc tree children");
	tr.child_count = 0;

	return tr;
}

void add_child(exp_tree_t *dest, exp_tree_t* src)
{
	if (++dest->child_count >= dest->child_alloc) {
		dest->child_alloc += 64;
		dest->child = realloc(dest->child,
			dest->child_alloc * sizeof(exp_tree_t *));
		if (!dest->child)
			fail("realloc tree children");
	}
	dest->child[dest->child_count - 1] = src;
}

void printout_tree(exp_tree_t et)
{
	int i;
	fprintf(stderr, "(%s", tree_nam[et.head_type]);
	if (et.tok && et.tok->start) {
		fprintf(stderr, ":");
		tok_display(stderr, *et.tok);
	}
	for (i = 0; i < et.child_count; i++) {
		fprintf(stderr, " ");
		fflush(stderr);
		printout_tree(*(et.child[i]));
	}
	fprintf(stderr, ")");
	fflush(stderr);
}
