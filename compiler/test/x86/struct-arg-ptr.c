typedef struct exp_tree {
	struct exp_tree *child[10];
	int foo[10]; 
} exp_tree_t;

typedef struct token {
	int a[10];
	int b[100];
} token_t;

int vt(exp_tree_t e)
{
	printf("%d\n", e.foo[0]);
}

exp_tree_t parse(token_t *t)
{
	exp_tree_t a;
	a.foo[0] = 123;
	exp_tree_t *et = &a;
	if (!vt(*et))
		;
}

main()
{
	parse(0x0);
}
