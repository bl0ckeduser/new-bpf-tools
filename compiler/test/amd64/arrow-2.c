typedef struct node {
	struct node* l;
	struct node* r;
	int val;
} node_t;

node_t *mknod(int val)
{
	node_t *n = malloc(sizeof(node_t));
	n->l = 0x0;
	n->r = 0x0;
	n->val = val;
	return n;
}

main()
{
	node_t *root = mknod(0), *ptr = root;
	int i;
	for (i = 0; i < 5; ++i)
		ptr = ptr->l = mknod(i + 1);
	printf("%d\n", root->l->l->l->l->l->val);
}
