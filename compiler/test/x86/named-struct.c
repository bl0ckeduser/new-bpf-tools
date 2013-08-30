struct bob {
	int x;
	int y;
};

main()
{
	struct bob foo;
	struct bob *bar = malloc(256);
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
