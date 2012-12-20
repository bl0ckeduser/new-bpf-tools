#include "tree.h"
#include "tokens.h"
#include <stdio.h>

/* TODO: constant folding ? other stuff ? */

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
	exp_tree_t *new, *new2;
	exp_tree_t *temp;
	int all_nums;
	token_t one = { TOK_INTEGER, "1", 1, 0, 0 };
	exp_tree_t one_tree = new_exp_tree(NUMBER, &one);

	for (i = 0; i < et->child_count; i++) {
		optimize(et->child[i]);
	}

	/* 
	 * BPF integer comparisons / bool system...
	 * This system considers an expression "true" whenever
	 * it is <= 0. This is because we use a "zbPtrTo var1, 0, var2"
	 * instruction for conditional branching. 
	 */

	/* Rewrite a <= b as a - b */
	if (et->head_type == LTE) {
		et->head_type = SUB;
	}

	/* Rewrite a < b as a - b + 1 */
	if (et->head_type == LT) {
		new = alloc_exptree(new_exp_tree(ADD, NULL));
		new2 = alloc_exptree(new_exp_tree(SUB, NULL));
		add_child(new2, et->child[0]);
		add_child(new2, et->child[1]);
		add_child(new, new2);
		add_child(new, alloc_exptree(one_tree));
		*et = *new;
	}

	/* Rewrite a >= b as b - a */
	if (et->head_type == GTE) {
		temp = et->child[0];
		et->child[0] = et->child[1];
		et->child[1] = temp;
		et->head_type = SUB;
	}

	/* Rewrite a > b as b - a + 1 */
	if (et->head_type == GT) {
		new = alloc_exptree(new_exp_tree(ADD, NULL));
		new2 = alloc_exptree(new_exp_tree(SUB, NULL));
		add_child(new2, et->child[1]);
		add_child(new2, et->child[0]);
		add_child(new, new2);
		add_child(new, alloc_exptree(one_tree));
		*et = *new;
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
