/*
 * ``findKth'' algorithm written for a CS course homework
 * (IIRC it's a recursive algorithm somewhat like quicksort
 * but which finds the k'th smallest element in an array.)
 *
 * Original file date: October 24, 2013
 *
 * Relevance for compiler:  Tests correct sizeof parsing precedence
 *
 * This time there are no changes to the file to make it compile :)
 * other than this stupid header
 */

#include <stdio.h>

int c = 0;

/* partitioning algorithm from COMP-250 Lecture #21 */
int partition(int *A, int start, int stop)
{
	int pivot = A[stop];
	int left = start;
	int right = stop - 1;
	int tmp;	

	while (left <= right) {
		while (left <= right && A[left] <= pivot) ++left;
		while (left <= right && A[right] >= pivot) --right;
		if (left < right) {
			tmp = A[right];
			A[right] = A[left];
			A[left] = tmp;
		}
	}

	tmp = A[stop];
	A[stop] = A[left];
	A[left] = tmp;

	return left;
}

int findKth(int *A, int start, int stop, int k)
{
	int *nw = malloc(sizeof(int) * (stop-start+1));
	int i;
	int res = 0;
	c = 0;
	for (i = 0; i <= stop; ++i)
		nw[i] = A[i];
	res = do_findKth(nw, start, stop, k);
	return res;
}

int do_findKth(int *A, int start, int stop, int k) {
	int p = partition(A, start, stop);
	c += stop - start + 1;
	if (p < k)
		return do_findKth(A, p+1, stop, k);
	if (p > k)
		return do_findKth(A, start, p - 1, k);
	return A[k];
}

main()
{
	int foo[] = {9, 10, 6, 7, 4, 12};
	int bar[] = {1, 7, 6, 4, 15, 10};
	int donald[] = { 9, 6, 8, 5, 7, 2, 3, 999, 48, 64, 47, 65 };
	int i;

	for (i = 0; i < sizeof donald / sizeof(int); ++i)
		printf("%d ", donald[i]);
	printf("\n");


	printf("%d\n",
		findKth(foo, 0, sizeof foo / sizeof(int) - 1, 0));

	printf("%d\n",
		findKth(bar, 0, sizeof bar / sizeof(int) - 1, 4));

	printf("%d\n",
		findKth(donald, 0, sizeof donald / sizeof(int) - 1, 
			0));
	
	for (i = 0; i < sizeof donald / sizeof(int); ++i) {
		printf("%d ",
			findKth(donald, 0, sizeof donald / sizeof(int) - 1, 
				i));
	}
	printf("\n");
}
