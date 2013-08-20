main()
{
	int *ptr = malloc(2500 * 4);
	int i;

	for (i = 0; i < 50; ++i)
		ptr[i * i] = i;

	for (i = 0; i < 50; ++i)
		printf("%d - %d\n", i, ptr[i * i]);

	free(ptr);
}
