struct bob {
	int x;
	int y;
};

print_bob(struct bob var) {
	printf("%d %d\n",
		var.x, var.y);
}

struct bob *bob_alloc()
{
	return malloc(256);
}

main()
{
	struct bob foo;
	struct bob *bar = bob_alloc();
	struct bob *herp[32];
	int i;

	for (i = 0; i < 32; ++i) {
		herp[i] = malloc(256);
		herp[i]->x = i;
	}

	foo.x = 123;
	printf("%d\n", foo.x);

	bar->x = 456;
	printf("%d\n", bar->x);

	for (i = 0; i < 10; ++i)
		printf("%d ", herp[i]->x);
	printf("\n");
}
