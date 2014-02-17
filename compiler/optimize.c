/* Optimize the syntax tree */

/* TODO: constant folding ? other stuff ? */

#include "tree.h"
#include "tokens.h"
#include <stdio.h>

int foldable(int ty)
{
	return 	   	   ty == ADD
			|| ty == SUB
			|| ty == MULT
			|| ty == DIV
			|| ty == CC_OR
			|| ty == CC_AND;
}

void optimize(exp_tree_t *et)
{
	int i = 0;
	exp_tree_t *below;
	exp_tree_t *right_child;

	for (i = 0; i < et->child_count; i++) {
		optimize(et->child[i]);
	}

	/* 
	 * Simplify nested binary trees,
	 * e.g. (SUB (SUB (NUMBER:3) (NUMBER:2)) (NUMBER:1))
	 * becomes (SUB (NUMBER:3) (NUMBER:2) (NUMBER:1))
	 * 
	 * This helps write tighter code, of course.
	 */
	if (et->child_count == 2
		&& et->child[0]->head_type == et->head_type
		&& foldable(et->head_type)) {
		below = et->child[0];
		right_child = et->child[1];
		et->child_count = 0;
		for (i = 0; i < below->child_count; i++)
			add_child(et, below->child[i]);
		add_child(et, right_child);
	}
}

