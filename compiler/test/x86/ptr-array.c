/*
 * Test correct array retrievals and assignments
 * for arrays of various types and base types / base sizes
 */

main()
{
	int i, j;
	char *stor;
	char *str_arr[32];
	char buf[32];

	for (i = 0; i < 10; ++i) {
		/* buf[j] is 1 byte wide,
		 * since buf is a char[] */
		for (j = 0; j < 10; ++j)
			buf[j] = 'a' + i;
		buf[j] = 0;
		stor = malloc(11);
		strcpy(stor, buf);

		/* stor is 32 bits wide
		 * since it is a pointer.
		 * str_arr[i] is 32 bits wide
		 * since str_arr is an array
		 * of pointers
		 */
		str_arr[i] = stor;
	}

	/* str_arr[i] is a pointer, it is 32 bits wide */
	for (i = 0; i < 10; ++i)
		puts(str_arr[i]);
}
