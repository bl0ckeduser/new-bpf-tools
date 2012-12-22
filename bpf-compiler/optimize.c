/* Optimize the syntax tree */

/* TODO: constant folding ? other stuff ? */

#include "tree.h"
#include "tokens.h"
#include <stdio.h>

int arith_type(int ty)
{
	return 	ty == ADD
			|| ty == SUB
			|| ty == MULT
			|| ty == DIV;
}

void optimize(exp_tree_t *et)
{
	int i = 0;
	exp_tree_t *below;

	for (i = 0; i < et->child_count; i++) {
		optimize(et->child[i]);
	}

	/* Simplify nested binary trees.
	 * e.g. (MULT (NUMBER:2) (MULT (NUMBER:3) (NUMBER:4)))
	 * becomes (MULT (NUMBER:2) (NUMBER:3) (NUMBER:4))
	 */
	if (et->child_count == 2 
		&& et->child[1]->head_type == et->head_type
		&& arith_type(et->head_type))
	{
		below = et->child[1];
		et->child_count = 1;
		for (i = 0; i < below->child_count; i++) {
			add_child(et, below->child[i]);
		}
	}
}

