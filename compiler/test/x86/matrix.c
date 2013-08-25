/*
 * Test a 2D array with declared dimensions
 * (nasty recursive offset calculations and other subtleties)
 */

main() {
	/* int mat[20][30] => 20 groups of 30 groups of ints
	 *
	 * mat[i] = mat + 30 * sizeof(int) * i
	 * mat[i][j] = mat[i] + sizeof(int) * j
	 * 	     = mat + 30 * sizeof(int) * i
	 *		+ sizeof(int) * j
	 */
	int mat[20][30];

	int i, j;

	for (i = 0; i < 20; ++i)
		for (j = 0; j < 30; ++j)
			mat[i][j] = i + j;
	printf("write ok\n");

	for (i = 0; i < 20; ++i)
		for (j = 0; j < 30; ++j)
			printf("%d+%d=%d\n", i, j, mat[i][j]);
	printf("read ok\n");

	/* XXX: segfaults on exit */
}
