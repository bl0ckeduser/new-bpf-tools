main()
{
	void *ptr = malloc(1024);
	char *ptr2 = ptr;
	int i;

	for (i = 0; i < 10; ++i)
		ptr2[i] = i;

	for (i = 0; i < 10; ++i)
		printf("%d ", ptr2[i]);
	printf("\n");
}
