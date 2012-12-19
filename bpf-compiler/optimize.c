#include "tree.h"
#include <stdio.h>

/* TODO: constant folding ? other stuff ? */

void optimize(exp_tree_t *et)
{
	int i = 0;
	exp_tree_t *below;
	int all_nums;

	/* Simplify nested binary trees.
	 * e.g. (MULT (NUMBER:2) (MULT (NUMBER:3) (NUMBER:4)))
	 * becomes (MULT (NUMBER:2) (NUMBER:3) (NUMBER:4))
	 */
	if (et->child_count == 2 
		&& et->child[1]->child_count == 2
		&& et->child[1]->head_type == et->head_type)
	{
		below = et->child[1];
		et->child_count = 1;
		add_child(et, below->child[0]);
		add_child(et, below->child[1]);
	}

	for (i = 0; i < et->child_count; i++) {
		optimize(et->child[i]);
	}
}
