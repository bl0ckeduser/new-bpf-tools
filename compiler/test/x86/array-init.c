main()
{
	int arr[] = {1, 2, 3, 3 + 1, 3 + 2, 3 + 3};
	int i, j;
	/* XXX: parser messes up if the decls above are sequenced */

	char *arr2[] = {"hello", "world", "hahah", "what", "a", "strange",
			"day"};

	int arr3[10] = {0};
	/* XXX: wrong behaviour ? */
	/* char *arr4[5] = {"ha"}; */

	for (i = 0; i < sizeof(arr) / sizeof(int); ++i)
		printf("%d ", arr[i]);
	printf("\n");

	for (i = 0; i < sizeof(arr2) / sizeof(char *); ++i)
		printf("%s ", arr2[i]);
	printf("\n");

	for (i = 0; i < sizeof(arr3) / sizeof(int); ++i)
		printf("%d ", arr3[i]);
	printf("\n");

/*
	for (i = 0; i < sizeof(arr4) / sizeof(char *); ++i)
		printf("%s ", arr4[i]);
	printf("\n");
*/

}
