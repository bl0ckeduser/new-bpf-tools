typedef struct node {
	struct node* l;
	struct node* r;
	int val;
} node_t;

node_t *mknod(int val)
{
	/* XXX: sizeof(node_t) */
	node_t *n = malloc(256);
	n->l = 0x0;	/* XXX: NULL */
	n->r = 0x0;	/* XXX: NULL */
	n->val = val;
	return n;
}

main()
{
	node_t *ptr = mknod(0);
	ptr->l = mknod(1);
	ptr->l->r = mknod(2);
	ptr->l->r->l = mknod(3);
	ptr->r = mknod(4);
	ptr->r->r = mknod(5);
	ptr->r->r->l = mknod(6);

	printf("%d\n", ptr->val);
	printf("%d\n", ptr->l->val);
	printf("%d\n", ptr->l->r->val);
	printf("%d\n", ptr->l->r->l->val);
	printf("%d\n", ptr->r->val);
	printf("%d\n", ptr->r->r->val);
	printf("%d\n", ptr->r->r->l->val);
}
