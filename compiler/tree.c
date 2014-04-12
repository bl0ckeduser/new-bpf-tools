/* Tree helper routines */

/* FIXME: crazy mallocs and memcpys, no garbage collection (!)
 *        a modern OS should clean up, but nevertheless, yeah */

#include "tree.h"
#include "tokens.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "general.h"

exp_tree_t null_tree;
char **tree_nam;

void init_tree()
{
	int i;

	null_tree.head_type = NULL_TREE;
	null_tree.tok = NULL;
	null_tree.child_count = 0;
	null_tree.child_alloc = 0;
	null_tree.child = NULL;

	/* sed 's/ = 0//g' | grep -v '/\*' | sed 's/\t/\t"/g' | sed 's/,$/",/g' */
	char* tree_nam_local[] = {
		"BLOCK",
		"ADD",
		"SUB",
		"MULT",
		"DIV",
		"ASGN",
		"IF",
		"WHILE",
		"NUMBER",
		"VARIABLE",
		"GT",
		"LT",
		"GTE",
		"LTE",
		"EQL",
		"NEQL",
		"INT_DECL",
		"BPF_INSTR",
		"NEGATIVE",
		"INC",
		"DEC",
		"POST_INC",
		"POST_DEC",
		"LABEL",
		"GOTO",
		"ARRAY",
		"ARRAY_DECL",
		"ARG_LIST",
		"PROC",
		"RET",
		"PROC_CALL",
		"ADDR",
		"DEREF",
		"STR_CONST",
		"MOD",
		"CC_OR",
		"CC_AND",
		"CC_NOT",
		"ARRAY_DIM",
		"DECL_CHILD",
		"DECL_STAR",
		"CAST",
		"BASE_TYPE",
		"CAST_TYPE",
		"CHAR_DECL",
		"TO_DECL_STMT",
		"TO_UNK",
		"TO_BLOCK",
		"BREAK",
		"ARG",
		"ARRAY_ADR",
		"STRUCT_DECL",
		"STRUCT_MEMB",
		"DEREF_STRUCT_MEMB",
		"NAMED_STRUCT_DECL",
		"SIZEOF",
		"LONG_DECL",
		"TERNARY",
		"COMPLICATED_INITIALIZER",
		"COMPLICATED_INITIALIZATION",
		"SEQ",
		"VOID_DECL",
		"BOR",
		"BXOR",
		"BAND",
		"BSL",
		"BSR",
		"CONTLAB",
		"CONTINUE",
		"MINGW_IOBUF",
		"PROTOTYPE",
		"PROTO_ARG",
		"SWITCH",
		"SWITCH_CASE",
		"SWITCH_DEFAULT",
		"SWITCH_BREAK",
		"ENUM_DECL",
		"PROC_CALL_MEMCPY",
		"EXTERN_DECL",
		"NULL_TREE"
	};

	tree_nam = malloc(sizeof(tree_nam_local));
	for (i = 0; i < sizeof(tree_nam_local) / sizeof(char *); ++i)
		tree_nam[i] = tree_nam_local[i];

}

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

exp_tree_t new_exp_tree(/*unsigned*/ int type, token_t* tok)
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
	dest->child[dest->child_count] = NULL;
}

/* lisp-inspired tree printouts, useful for debugging
 * the parser and codegenerator. */
void printout_tree(exp_tree_t et)
{
	int i;
	fprintf(stderr, "(%s", tree_nam[et.head_type]);
	if (et.tok && et.tok->start) {
		fprintf(stderr, ":");
		tok_display(*et.tok);
	}
	for (i = 0; i < et.child_count; i++) {
		fprintf(stderr, " ");
		fflush(stderr);
		printout_tree(*(et.child[i]));
	}
	fprintf(stderr, ")");
	fflush(stderr);
}

