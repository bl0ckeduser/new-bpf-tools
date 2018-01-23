typedef struct exp_tree {
        char head_type;
        int child_count;
        int child_alloc;
        struct exp_tree **child;
} exp_tree_t;

typedef struct {
	int foo;
	int bar;
} token_t;

int test(exp_tree_t *et)
{
	token_t* make_fake_tok(char *s);
	printf("%d\n", et->child_count);
}

main()
{
	exp_tree_t a;
	a.child_count = 123456789;
	test(&a);
}
