/*
 * Integer-constant folder
 *
 * Adapted from symdiff, another project of
 * mine which is a small computer algebra system
 * which has some code in common with this project
 * (tree implementation, parser, tokenizer)
 */

#include "tokens.h"
#include "tree.h"
#include "general.h"
#include <string.h>
#include <stdlib.h>

/* 
 * Check for an arithmetic operation tree 
 */
int arith_type(int ty)
{
	return 	ty == ADD
			|| ty == SUB
			|| ty == MULT
			|| ty == DIV;
}

/* 
 * Convert token's string to integer 
 */
int tok2int(token_t *t)
{
	char buf[1024];
	int i;
	strncpy(buf, t->start, t->len);
	buf[t->len] = 0;
	sscanf(buf, "%d", &i);
	return i;
}

token_t* make_fake_tok(char *s)
{
	token_t *t = malloc(sizeof(token_t));
	if (!t)
		fail("malloc(sizeof(token_t))");
	t->start = malloc(strlen(s) + 1);
	strcpy(t->start, s);
	t->len = strlen(s);
	return t;
}

void constfold(void *ptr)
{
	int i = 0;
	int q;
	int chk, chk2;
	int a, b;
	int val;
	exp_tree_t *below;
	exp_tree_t *right_child;
	exp_tree_t new, num;
	exp_tree_t *new_ptr;
	char buf[128];
	exp_tree_t *et = (exp_tree_t *)ptr;

	for (i = 0; i < et->child_count; i++)
		constfold(et->child[i]);

	/* e.g. (DIV 6 3) -> (NUMBER 2) */
	if (et->head_type == DIV
	&& et->child_count == 2
	&& et->child[0]->head_type == NUMBER
	&& et->child[1]->head_type == NUMBER) {
		a = tok2int(et->child[0]->tok);
		b = tok2int(et->child[1]->tok);
		if (b != 0 && a % b == 0) {
			et->child_count = 0;
			et->head_type = NUMBER;
			sprintf(buf, "%d", a / b);
			et->tok = (token_t *)make_fake_tok(buf);
			return;
		}
	}

	/*
	 * Fold trivial arithmetic constants
	 */
	if (et->head_type == MULT
	|| et->head_type == ADD
	|| et->head_type == SUB
	|| et->head_type == NEGATIVE) {
		chk = 1;
		for (i = 0; i < et->child_count; ++i)
			if (et->child[i]->head_type != NUMBER) {
				chk = 0;
				break;
			}

		if (chk) {
			val = tok2int(et->child[0]->tok);
			for (i = 1; i < et->child_count; ++i) {
				switch(et->head_type) {
					case MULT:
					val *= tok2int(et->child[i]->tok);
					break;

					case ADD:
					val += tok2int(et->child[i]->tok);
					break;

					case SUB:
					val -= tok2int(et->child[i]->tok);
					break;
				}
			}

			if (et->head_type == NEGATIVE)
				val *= -1;

			et->head_type = NUMBER;
			sprintf(buf, "%d", val);
			et->tok = (token_t *)make_fake_tok(buf);
			et->child_count = 0;
		} else {
			/* 
			 * Simplify mixed number/non-number
			 * to one number and the rest 
			 */
			chk = 0;
			chk2 = 0;
			for (i = 0; i < et->child_count; ++i) {
				if (et->child[i]->head_type == NUMBER)
					chk++;
				if (et->child[i]->head_type != NUMBER)
					chk2 = 1;
			}

			if ((chk > 1 || (chk == 1 && et->child[0]->head_type != NUMBER))
				&& chk2
				&& (et->head_type == ADD || et->head_type == MULT)) {
				new = new_exp_tree(et->head_type, et->tok);
				val = et->head_type == ADD ? 0 : 1;
				for (i = 0; i < et->child_count; ++i) {
					if (et->child[i]->head_type == NUMBER) {
						switch(et->head_type) {
							case MULT:
							val *= tok2int(et->child[i]->tok);
							break;

							case ADD:
							val += tok2int(et->child[i]->tok);
							break;
						}
					}
				}
				sprintf(buf, "%d", val);
				num = new_exp_tree(NUMBER, make_fake_tok(buf));
				new_ptr = alloc_exptree(new);
				add_child(new_ptr, alloc_exptree(num));
				
				for (i = 0; i < et->child_count; ++i)
					if (et->child[i]->head_type != NUMBER)
						add_child(new_ptr, 
							alloc_exptree(copy_tree(*(et->child[i]))));

				/* overwrite tree data */
				memcpy(et, new_ptr, sizeof(exp_tree_t));

				return;
			}
		}
	}

	/* Simplify nested binary trees,
	 * e.g. (SUB (SUB (NUMBER:3) (NUMBER:2)) (NUMBER:1))
	 * becomes (SUB (NUMBER:3) (NUMBER:2) (NUMBER:1))
	 */
	if (et->child_count == 2
		&& et->child[0]->head_type == et->head_type
		&& arith_type(et->head_type)) {
		below = et->child[0];
		right_child = et->child[1];
		et->child_count = 0;
		for (i = 0; i < below->child_count; i++)
			add_child(et, below->child[i]);
		add_child(et, right_child);
		return;
	}

	/* (MUL (MUL xxx) yyy zzz) => (MUL xxx yyy zzz) */
	if (et->child_count >= 2
		&& et->child[0]->head_type == et->head_type
		&& et->child[0]->child_count
		&& (et->head_type == MULT || et->head_type == ADD)) {

		new = new_exp_tree(et->head_type, NULL);
		new_ptr = alloc_exptree(new);

		below = et->child[0];	

		for (q = 0; q < below->child_count; ++q)
			add_child(new_ptr, below->child[q]);
			
		for (q = 1; q < et->child_count; ++q)
			add_child(new_ptr, et->child[q]);

		memcpy(et, new_ptr, sizeof(exp_tree_t));
		
		return;
	} 

	/* (MUL xxx (MUL yyy)) => (MUL xxx yyy) */
	if (et->child_count >= 2
		&& et->child[et->child_count - 1]->head_type
		== et->head_type
		&& (et->head_type == MULT || et->head_type == ADD)) {
		below = et->child[et->child_count - 1];
		if (below->child_count) {
			--(et->child_count);
			for (i = 0; i < below->child_count; i++)
				add_child(et, below->child[i]);
			return;
		}
	}
}
