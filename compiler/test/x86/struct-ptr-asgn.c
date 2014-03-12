typedef struct exp_tree {
        char head_type;
        int child_count;
        int child_alloc;
        struct exp_tree **child;
} exp_tree_t;

thing(exp_tree_t *p)
{
	exp_tree_t a;
	a.head_type = 123;
	exp_tree_t *q = &a;
	*p = *q;
}

main()
{
	exp_tree_t a, b;
	exp_tree_t *p = &a, *q = &b;

	thing(p);
	printf("%d\n", a.head_type);
}
