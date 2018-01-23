/* copy an array of ints */
int *clone(int *arr, int len)
{
	int *nw = malloc(len * sizeof(int));
	int i;
	if (!nw) {
		puts("malloc failed");
		exit(1);
	}
	for (i = 0; i < len; ++i)
		nw[i] = arr[i];
	return nw;
}

merge(int *arr, int start, int end, int len)
{
	int mid = (start + end) / 2;
	int a = start;
	int b = mid + 1;
	int i = start;
	int *src = clone(arr, len);
	while (i <= end)
		if (a <= mid && (b > end || src[a] < src[b]))
			arr[i++] = src[a++];
		else
			arr[i++] = src[b++];
	free(src);
}

mergesort(int *arr, int start, int end, int len)
{
	int mid = (start + end) / 2;
	int i;
	if (end - start >= 1) {
		mergesort(arr, start, mid, len);
		mergesort(arr, mid + 1, end, len);
		merge(arr, start, end, len);
	}
}

main()
{
	int i;
	int len = 4000;	/* arbitrary */
	int *arr = malloc(len * sizeof(int));

	/* make random entries */
	for (i = 0; i < len; ++i)
		arr[i] = rand() % 100;

	/* show sort input */
	for (i = 0; i < len; ++i)
		printf("%d ", arr[i]);
	printf("\n");

	/* run the sort */
	mergesort(arr, 0, len - 1, len);

	/* show sort output */
	for (i = 0; i < len; ++i)
		printf("%d ", arr[i]);
	printf("\n");

}

/*

./compile-run-x86.sh test/x86/merge-sort.c >/tmp/sort_test_tmp.$$
head -1 /tmp/sort_test_tmp.$$ | tr ' ' '\n' | grep -v '^$' | sort -n | tr '\n' ' ' | sum
tail -1 /tmp/sort_test_tmp.$$ | tr -d '\n' | sum
rm /tmp/sort_test_tmp.$$

*/
