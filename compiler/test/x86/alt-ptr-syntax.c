/*
 * alternate pointer syntax in procedure arguments
 * the specific part being tested is "char *vec[]"
 * in the foo procedure's arglist.
 * the old code gave this error:
 *
 *    void foo(char *vec[], int l)
 *                      ^
 *    error: line 8: , expected
 *
 */

void foo(char *vec[], int l)
{
	int i;
	for (i = 0; i < l; ++i)
		puts(vec[i]);
}

main()
{
	/* XXX: char *vec[] ? */
	char **vec = malloc(5 * sizeof(char *));
	int i;
	for (i = 0; i < 5; ++i) {
		vec[i] = malloc(32);
		sprintf(vec[i], "ah yes, i=%d", i);
	}
	foo(vec, 5);
}
