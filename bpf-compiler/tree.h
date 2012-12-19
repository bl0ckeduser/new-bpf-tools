#ifndef TREE_H
#define TREE_H

#include <stdlib.h>
#include "tokenizer.h"

enum {		/* head_type */
	BLOCK = 0,
	ADD,
	SUB,
	MULT,
	DIV,
	ASGN,
	IF,
	WHILE,
	NUMBER,
	VARIABLE,
	GT,
	LT,
	GTE,
	LTE,
	EQL,
	NEQL,
	INT_DECL,
	BPF_INSTR,
	NEGATIVE,
	INC,
	DEC,
	/* special */
	NULL_TREE
};

static char* tree_nam[] = {
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
	"NULL_TREE"
};


typedef struct exp_tree {
	char head_type;
	token_t* tok;
	unsigned int child_count;
	unsigned int child_alloc;
	struct exp_tree **child;
} exp_tree_t;

static exp_tree_t null_tree = { NULL_TREE, NULL, 0, 0, NULL };

extern void add_child(exp_tree_t *dest, exp_tree_t src);
extern exp_tree_t new_exp_tree(unsigned int type, token_t* tok);

#endif
