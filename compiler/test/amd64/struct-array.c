/*
 * Test 1-dimensional and 2-dimensional
 * arrays of struct
 */

typedef struct bob {
	int x;
	int y;
	int prod;
} bob_t;

write_matrix(bob_t *mat, int x, int y, int prod)
{
	mat->x = x;
	mat->y = y;
	mat->prod = prod;
}

main()
{
	struct bob foo[32];
	bob_t matrix[10][10];
	int i, j;

	/* 1d array test */
	for (i = 0; i < 32; ++i)
		foo[i].x = i;
	for (i = 0; i < 10; ++i)
		printf("%d ", i, foo[i].x);
	printf("\n");

	/* 2d array test */
	for (i = 0; i < 10; ++i)
		for (j = 0; j < 10; ++j)
			write_matrix(&matrix[i][j], i, j, i * j);
	for (i = 0; i < 10; ++i)
		for (j = 0; j < 10; ++j)
			printf("%d, %d, %d\n",
				matrix[i][j].x, matrix[i][j].y,
					matrix[i][j].prod);
}
