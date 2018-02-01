/*
 * ProjectEuler problem 77
 *  -- a C solution by blockeduser
 *  originally written October 2012
 * very slightly tweaked to compile
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* note, this assumes a 64-bit compiler which uses 64-bit longs !!!! */
typedef long largish_uint;

/*static*/ int run_out = 0;


typedef struct node {
	struct node *left;
	struct node *right;
	largish_uint* val;
	int val_alloc;
	int siz;
} node_t;

void fail(char *msg)
{
	fprintf(stderr, "error: %s\n", msg);
	exit(1);
}

node_t* new_node(largish_uint v, int siz)
{
	node_t *n = malloc(sizeof(node_t));
	if (!n)
		fail("malloc");
	n->val = malloc(16 * sizeof(largish_uint));
	if (!n->val)
		fail("malloc");
	n->val_alloc = 16;
	n->left = n->right = NULL;
	n->siz = siz;
	*(n->val) = v;
	
	return n;
}

char prime_memo[2000];

int prime_raw(int n)
{
        int i = 1;
        while(++i <= n / 2)
                if(n % i == 0)
                        return 0;
        return n > 1;
}

int prime(n)
{
	if (n >= 2000)
		return prime_raw(n);

	/* memoization */
	if (!prime_memo[n])
		prime_memo[n] = prime_raw(n) + 1;
	return prime_memo[n] - 1;
}

void add_val(node_t *nod, largish_uint v)
{
	if (++nod->siz > nod->val_alloc) {
		nod->val_alloc += 16;
		nod->val = realloc(nod->val,
			nod->val_alloc * sizeof(largish_uint));
		if (!nod->val)
			fail("realloc");
	}
	nod->val[nod->siz - 1] = v;
}

traverse(node_t *nod, node_t *parent)
{
	largish_uint prod;
	int i, j;

	if (nod->left)
		traverse(nod->left, nod);

	if (nod->right)
		traverse(nod->right, nod);

	/* when a pair of leaves is reached, prune it
	 * and add its products to its parent's list */
	if (!nod->left && !nod->right && parent) {
		for (i = 0; i < parent->left->siz; i++) {
			for (j = 0; j < parent->right->siz; j++) {
				prod = parent->left->val[i] *
					parent->right->val[j];
				add_val(parent, prod);
			}
		}

		free(parent->left->val);
		free(parent->left);
		free(parent->right->val);
		free(parent->right);
		parent->left = parent->right = NULL;
	}
}

largish_uint* build_memo[1000];
int build_memo_siz[1000];

void free_build_memo()
{
	int i;
	for (i = 0; i < 1000; i++)
		if (build_memo[i])
			free(build_memo[i]);
}

node_t* build(node_t *root, largish_uint n)
{
	node_t *nod;
	node_t *my_root;
	extern node_t* build_raw(node_t *root, largish_uint n);
	int i;		

	if (n > 1000)
		return build_raw(root, n);
	else if (!build_memo[n]) {
		my_root = new_node(n, prime(n));
		nod = build_raw(my_root, n);

		build_memo[n] = malloc(nod->siz * 
			sizeof(largish_uint));
		if (!build_memo[n]) {
			printf("ran out of memoization space\n");
			return build_raw(root, n);
		}
		memcpy(build_memo[n], nod->val, 
			nod->siz * sizeof(largish_uint));
		build_memo_siz[n] = nod->siz;

		free(nod->val);
		free(nod);
	}
	
	if (root->siz != prime(n))
		return build_raw(root, n);

	if (root->val_alloc < build_memo_siz[n]) {
		root->val_alloc = build_memo_siz[n];
		root->val = realloc(root->val,
			root->val_alloc * sizeof(largish_uint));
		if (!root->val)
			fail("list realloc");
	}
	
	memcpy(root->val, build_memo[n], 
		build_memo_siz[n] * sizeof(largish_uint));

	root->siz =  build_memo_siz[n];

	return root;
}

node_t* build_raw(node_t *root, largish_uint n)
{
	largish_uint a;
	int i, j, k;
	node_t *nod;
	node_t *l, *r;

	largish_uint* master;
	int ma = 16;
	int siz = 0;

	largish_uint* master_uniq;
	int mua = 16;
	int usiz = 0;

	int orig_siz = root->siz;

	master = malloc(16 * sizeof(largish_uint));
	master_uniq = malloc(16 * sizeof(largish_uint));
	
	if (!master || !master_uniq)
		fail("malloc list");

	*(root->val) = n;
	root->siz = orig_siz;
	*master_uniq = n;
	usiz = orig_siz;

	for (a = n / 2; a > 0; a--) {
		/* build the basic tree */
		*(root->val) = n;
		root->siz = orig_siz;
		root->left = build(new_node(a, prime(a)), a);
		root->right = build(new_node(n - a, prime(n - a)),
			n - a);

		/* simplify it */
		while (root->left) {
			traverse(root->left, root);
			if (root->right)
				traverse(root->right, root);
		}

		/* add the tree's list to the total list */
		for (i = 0; i < root->siz; i++) {
			if (++siz > ma) {
				ma += 16;
				master = realloc(master,
					ma * sizeof(largish_uint));
				if (!master)
					fail("realloc list");
			}
			master[siz - 1] = root->val[i];
		}
	}

	/* remove duplicates from total list */
	for (i = 0; i < siz; i++) {
		k = 0;
		for (j = 0; j < usiz; j++)
			if (master_uniq[j] == master[i])
			{
				k = 1;
				break;
			}

		if (!k) {
			if (++usiz > mua) {
				mua = usiz + 16;
				master_uniq = realloc(master_uniq,
					mua * sizeof(largish_uint));
				if (!master_uniq)
					fail("realloc list");
			}
			master_uniq[usiz - 1] = master[i];
		}
	}

	/* move total list register to the node's
 	 * list */
	if (root->val_alloc < usiz) {
		root->val_alloc = usiz + 16;
		root->val = realloc(root->val,
			root->val_alloc * sizeof(largish_uint));
		if (!root->val)
			fail("realloc list");
	}

	for (i = 0; i < usiz; i++) {
		if (master_uniq[i] < 0) {
			printf("bad things have happened !\n");
			exit(1);
		}
		root->val[i] = master_uniq[i];
	}
	root->siz = usiz;

	free(master);
	free(master_uniq);

	return root;
}

do_it(int n)
{
	node_t *root = new_node(n, 0);
	node_t *simp = build_raw(root, n);
	int s = simp->siz;

	free(simp->val);
	free(simp);

	return s;
}

int main(int argc, char** argv)
{
	int t;
	int n;

	for (n = 1; n < 128; n++) {
		printf("%d - %d\n", n, t = do_it(n));
		if (t > 5000) {
			printf("solution: %d\n", n);
			break;
		}
	}

	free_build_memo();
	return 0;
}
