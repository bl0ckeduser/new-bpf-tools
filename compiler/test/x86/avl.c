/*
 * Trying to practice AVL trees
 *
 * This specific compiler-testing sample,
 * based on some recent noob AVL code I wrote,
 * insert a bunch of keys, and at the end barfs out
 * the final AVL out as an S-expression. to see
 * the tree graphically do:
 *
 *	 ./avl | drawtree avl.png
 *
 * where `drawtree' is another program by
 * my noobself, availabel on my website.
 *
 * Note that this file tests expressions
 * like
 *		&(a->b)
 *
 * specifically in lines like this one:
 *
 * 	insertion_point = &(node->left);
 *
 * Tue Jan 21 21:36:58 EST 2014
 */

#include <stdlib.h>
#include <stdio.h>

long null = 0x0;	/* XXX: if real headers no more of this hack */

typedef struct avl_node {
	int height;
	int value;
	struct avl_node* left;
	struct avl_node* right;
	struct avl_node* parent;
} avl_node_t;

avl_node_t *root;

avl_node_t *imbalance = 0x0; /* XXX: NULL */

avl_node_t* makenode(int value)
{
	avl_node_t *nod = malloc(sizeof(avl_node_t));
	if (!nod) {
		printf("malloc failed in makenode()\n");
		exit(1);
	}
	nod->left = null;
	nod->right = null;
	nod->height = 0;
	nod->value = value;
	nod->parent = null;
	return nod;
}

void insert(avl_node_t* root, int val)
{
	avl_node_t* node = root;
	avl_node_t** insertion_point = null;
	avl_node_t* new_node;

	for (;;) {
		if (val < node->value) {
			if (node->left)
				node = node->left;
			else {
				insertion_point = &(node->left);
				break;
			}
		} else if (val > node->value) {
			if (node->right)
				node = node->right;
			else {
				insertion_point = &(node->right);
				break;
			}
		}
		else
			break;
	}

	if (!insertion_point) {
		printf("Fuckup in insertion routine\n");
		exit(1);
	}
	new_node = makenode(val);
	new_node->parent = node;
	*insertion_point = new_node;

	/*	========== rebalance the avl tree ========= */

	for (;;) {

		imbalance = null;
		populate_heights(root);

		if (imbalance) {
			/* right-outside case */
			if (val > imbalance->value && val > imbalance->right->value)
				left_rotate(imbalance);
			/* left-outside case */
			else if (val < imbalance->value && val < imbalance->left->value) {
				right_rotate(imbalance);
			}
			/* right-left inside case */
			else if (val > imbalance->value && val < imbalance->right->value) {
				right_rotate(imbalance->right);
				left_rotate(imbalance);
			}
			/* left-right inside case */
			else if (val < imbalance->value && val > imbalance->left->value) {
				left_rotate(imbalance->left);
				right_rotate(imbalance);
			}
		}
		else
			break;

	}

	return 0;
}

void left_rotate(avl_node_t *k1)
{
	int orig_val = k1->value;

	avl_node_t *k1_new = makenode(0);
	avl_node_t *k2_new = makenode(0);
	avl_node_t *a_new = makenode(0);
	avl_node_t *b_new = makenode(0);
	avl_node_t *c_new = makenode(0);
	memcpy(k1_new, k1, sizeof(avl_node_t));
	memcpy(k2_new, k1->right, sizeof(avl_node_t));
	k1_new->left = k1_new->right = k2_new->left = k2_new->right = null;

	if (k1->left) {
		memcpy(a_new, k1->left, sizeof(avl_node_t));
		k1_new->left = a_new;
		a_new->parent = k1_new;
	}

	if (k1->right->left) {
		memcpy(b_new, k1->right->left, sizeof(avl_node_t));
		k1_new->right = b_new;
		b_new->parent = k1_new;
	}

	if (k1->right->right) {
		memcpy(c_new, k1->right->right, sizeof(avl_node_t));
		k2_new->right = c_new;
		c_new->parent = k2_new;
	}

	k1_new->parent = k2_new;
	k2_new->left = k1_new;

	k2_new->parent = k1->parent;
	memcpy(k1, k2_new, sizeof(avl_node_t));
}

void right_rotate(avl_node_t *k2)
{
	int orig_val = k2->value;

	avl_node_t *k1_new = makenode(0);
	avl_node_t *k2_new = makenode(0);
	avl_node_t *a_new = makenode(0);
	avl_node_t *b_new = makenode(0);
	avl_node_t *c_new = makenode(0);
	memcpy(k2_new, k2, sizeof(avl_node_t));
	memcpy(k1_new, k2->left, sizeof(avl_node_t));
	k1_new->left = k1_new->right = k2_new->left = k2_new->right = null;

	if (k2->left->left) {
		memcpy(a_new, k2->left->left, sizeof(avl_node_t));
		k1_new->left = a_new;
		a_new->parent = k1_new;
	}

	if (k2->left->right) {
		memcpy(b_new, k2->left->right, sizeof(avl_node_t));
		k2_new->left = b_new;
		b_new->parent = k2_new;
	}

	if (k2->right) {
		memcpy(c_new, k2->right, sizeof(avl_node_t));
		k2_new->right = c_new;
		c_new->parent = k2_new;
	}

	k2_new->parent = k1_new;
	k1_new->right = k2_new;

	k1_new->parent = k2->parent;

	memcpy(k2, k1_new, sizeof(avl_node_t));
}

int populate_heights(avl_node_t *node)
{
	int max = 0;
	int ld = -1;
	int rd = -1;
	int nh;

	if (node->left && (nh = populate_heights(node->left) + 1) > max)
		max = nh;
	if (node->right && (nh = populate_heights(node->right) + 1) > max)
		max = nh;

	if (node->left)
		ld = node->left->height;
	if (node->right)
		rd = node->right->height;
	if (!imbalance && abs(ld - rd) > 1)
		imbalance = node;

	return node->height = max;
}

void do_dump(avl_node_t* node)
{
	populate_heights(node);
	dump(node, 0);
}

void dump(avl_node_t* node, int depth)
{
	int i;
	int leaf = !(node->left != null || node->right != null);

	if (!leaf)
		printf("(");
	printf("v=%d,h=%d ", node->value, node->height);
	fflush(stdout);

	if (node->left) {
		fflush(stdout);
		dump(node->left, depth + 1);
	} else if (!leaf) {
		printf(" X,-1 ");	/* null leaf */
	}

	if (node->right) {
		fflush(stdout);
		dump(node->right, depth + 1);
		fflush(stdout);
	} else if (!leaf) {
		printf(" X,-1 ");	/* null leaf */
	}

	if (!leaf)
		printf(")");
	printf(" ");

	if (depth == 0)
		printf("\n");
}

main(int argc, char *argv[])
{
	int nodes[] = {3, 7, 2, 4, 8, 9, 10, 13, 15, 11, 12, 30};
	int i, j;

	/*
	 * Create the root
	 */
	root = makenode(nodes[0]);

	/*
	 * Insert subsequent nodes
	 */
	for (j = 1; j < sizeof(nodes) / sizeof(int); ++j)
		insert(root, nodes[j]);

	/*
	 * Dump final AVL tree
	 */
	do_dump(root);
}
