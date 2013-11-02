main()
{
	struct foo {
		int bar;
	};

	int max;
	struct foo* a = malloc(sizeof(struct foo));
	struct foo* b = malloc(sizeof(struct foo));

	a->bar = 123;
	b->bar = 456;

	printf("%d%d\n", a->bar, b->bar);

	max = a->bar > b->bar ? a->bar : b->bar;
	printf("%d\n", max);
}
