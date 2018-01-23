/*
 * This tests assignment automatic type conversions
 * and also assignments between same-sized types
 * XXX: why not also test arithmetic
 */

main() {
	char a, b;
	int x, y;
	char cd = 'a';
	char *cptr = &cd, *cptr2;
	char **c_ptr_ptr = &cptr;
	char carr[32];
	int *iptr = &x, *iptr2;
	int **i_ptr_ptr = &iptr;
	int iarr[32];

	/* char <- int */
	x = 'B';
	b = x;
	putchar(b);
	b = *(char *)&x;
	putchar(b);
	b = 'B';
	putchar(b);
	printf("\n");

	/* int <- char */
	carr[15] = b = 67;
	x = b;
	printf("%d\n", x);
	x = *cptr;
	printf("%d\n", x);
	x = **c_ptr_ptr;
	printf("%d\n", x);
	x = carr[15];
	printf("%d\n", x);

	/* int <- int */
	iarr[7] = x = 123;
	y = x;
	printf("%d\n", y);
	y = 456;
	printf("%d\n", y);
	y = *iptr;
	printf("%d\n", y);
	y = **i_ptr_ptr;
	printf("%d\n", y);
	y = iarr[7];
	printf("%d\n", y);

	/* char <- char */
	b = 'B';
	a = b;
	putchar(a);
	a = *(char *)&b;
	putchar(a);
	a = 'B';
	putchar(a);
	printf("\n");

	/* int * <- int * */
	iptr = &x;
	iptr2 = iptr;
	printf("%d\n", iptr == iptr2);
}
