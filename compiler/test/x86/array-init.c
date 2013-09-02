main()
{
	int arr[] = {1, 2, 3, 3 + 1, 3 + 2, 3 + 3};
	int i;
	/* XXX: parser messes up if the decls above are sequenced */

	char *arr2[] = {"hello", "world", "hahah", "what", "a", "strange",
			"day"};

	for (i = 0; i < sizeof(arr) / sizeof(int); ++i)
		printf("%d ", arr[i]);
	printf("\n");

	for (i = 0; i < sizeof(arr2) / sizeof(char *); ++i)
		printf("%s ", arr2[i]);
	printf("\n");
}
