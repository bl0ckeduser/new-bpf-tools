#ifndef TREE_H
#define TREE_H

enum {		/* head_type */
	BLOCK,
	ADD,
	SUB,
	MULT,
	DIV,
	ASGN,
	IF,
	WHILE,
	NUMBER,
	VARIABLE
};

typedef struct exp_tree {
	char head_type;
	int head_dat;
	unsigned int child_count;
	unsigned int child_alloc;
	struct exp_tree *child;
} exp_tree_t;

extern void add_child(exp_tree_t dest, exp_tree_t src);
extern exp_tree_t new_exp_tree(unsigned int type, unsigned int dat);

#endif
