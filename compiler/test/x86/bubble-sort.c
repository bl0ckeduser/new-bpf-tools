/*
 * Bubble sort
 */

bubblesort(int *a, int len) {
	int i, j, tmp;
	int swaps = 0;
	for (i = 0; i < len; ++i) {
		for (j = 0; j < len - i - 1; ++j) {
			if (a[j + 1] < a[j]) {
				tmp = a[j];
				a[j] = a[j + 1];
				a[j + 1] = tmp;
			}
		}
	}
}

main()
{
	int* a = malloc(50 * sizeof(int));
	int i, j = 0;
	/*
	 * Note: rand() will give the same thing everytime you
	 * run the program on a given system because
	 * i didn't bother to seed it. this makes testing
	 * against gcc-compiled output possible.
	 */
	for (i = 0; i < 50; ++i)
		a[i] = rand() % 1000;
	bubblesort(a, 50);
	for (i = 0; i < 50; ++i)
		printf("%d ", a[i]);
	printf("\n");
}
