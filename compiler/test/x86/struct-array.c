struct bob {
	int x;
	int y;
};

main()
{
	struct bob foo[32];
	int i;
	for (i = 0; i < 32; ++i)
		foo[i].x = i;
	for (i = 0; i < 10; ++i)
		printf("%d ", i, foo[i].x);
	printf("\n");
}
