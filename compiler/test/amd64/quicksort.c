/*
 * quicksort code written in anticipation of a job interview
 * march 19, 2014
 */

void exchg(int *a, int *b)
{
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

int partition(int *arr, int start, int end)
{
	int a = start;
	int b = end - 1;
	int pivot = arr[end];
	int i;
	int temp;

	while (a <= b) {
		while (a <= b && arr[a] <= pivot) ++a;
		while (a <= b && arr[b] >= pivot) --b;
		if (a < b)
			exchg(&arr[a], &arr[b]);
	}
	exchg(&arr[end], &arr[a]);
	return a;
}

void quicksort(int *arr, int start, int end)
{
	int p;
	if (end - start <= 1) {
		return;
	} else {
		p = partition(arr, start, end);
		quicksort(arr, 0, p - 1);
		quicksort(arr, p + 1, end);
	}
}

main()
{
	int i;
	/*
	 * Sun May 11 12:11:36 EDT 2014
	 *
	 * prior to a recent fix, this initializer caused
	 * the compiler to run out of registers. anyway it
	 * was no big deal; it was literally throwing memory
	 * out the window and the issue was fixed with four extra lines.
	 */
	int a[] = {1, 3, 2, 5, 7, 6, 0, 8, 9, 10, 7, 7, 8, 19, 20, 18, 17, -3, -2, -5, -3, -2};
	quicksort(a, 0, sizeof(a) / sizeof(int) - 1);
	for (i = 0; i < sizeof(a) / sizeof(int); ++i) {
		printf("%d ", a[i]);
	}
	printf("\n");
}
