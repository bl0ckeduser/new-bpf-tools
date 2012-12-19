#include "tree.h"

exp_tree_t new_exp_tree(unsigned int type, unsigned int dat)
{
	exp_tree_t tr;
	tr.head_type = type;
	tr.head_dat = dat;
	tr.child_alloc = 64;
	tr.child = malloc(64 * sizeof(exp_tree_t));
	if (!tr.child)
		fail("malloc tree children");
	tr.child_count = 0;
}

void add_child(exp_tree_t dest, exp_tree_t src)
{
	if (++dest.child_count > dest.child_alloc) {
		dest.child_alloc += 64;
		dest.child = realloc(dest.child,
			dest.child_alloc * sizeof(exp_tree_t));
		if (!dest.child)
			fail("realloc tree children");
	}
	dest.child[dest.child_count - 1] = src;
}
