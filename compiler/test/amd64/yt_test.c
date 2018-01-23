/* 
 * http://www.youtube.com/watch?v=Y1UdEQHsoLk
 * august 17 2014
 */

recursive_fib(int n) {
	if (n <= 1)
		return n;
	else
		return recursive_fib(n - 1) + recursive_fib(n - 2);
}

typedef struct tree_node_struct {
	struct tree_node_struct* left;
	struct tree_node_struct* right;
	int val;
} tnode_t;

tnode_t *mk_tnode() {
	return malloc(sizeof(tnode_t));
}

main() {
	int x = 1 + 2 * 3;
	int y = x * x;
	tnode_t *ptr = mk_tnode();
	ptr->left = mk_tnode();
	ptr->left->left = mk_tnode();
	ptr->left->left->right = mk_tnode();
	ptr->left->left->right->val = 123;

	printf("node val = %d\n",
		ptr->left->left->right->val);

	printf("x=%d, y=%d\n", x, y);

	for (x = 0; x < 10; ++x) {
		printf("fib(%d) = %d\n", x, recursive_fib(x));
	}
}


