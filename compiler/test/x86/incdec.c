main()
{
	int i;
	char chars[8];
	int *int_ptr;
	char *char_ptr;
	int ints[8];

	sprintf(chars, "hello");

	for (i = 0; i < 8; ++i)
		ints[i] = i + i;

	/*
	 * char_ptr is a pointer to char.
	 * this means it is itself
	 * a 4-byte quantity, and
	 * that because of
	 * pointer arithmetic,
	 * pre-incrementing it
	 * increases it by exactly 1.
	 */
	char_ptr = &chars[0];
	for (i = 0; i < strlen(chars); ++i)
		puts(++char_ptr);

	char_ptr = &chars[5];
	for (i = 0; i < strlen(chars); ++i)
		puts(--char_ptr);


	/*
	 * int_ptr is a pointer to int.
	 * this means it is itself
	 * a 4-byte quantity, and
	 * that because of
	 * pointer arithmetic,
	 * pre-incrementing it
	 * increases it by exactly 4.
	 */
	int_ptr = &ints[0];
	for (i = 0; i < 5; ++i)
		printf("%d\n", *++int_ptr);

	int_ptr = &ints[5];
	for (i = 0; i < 5; ++i)
		printf("%d\n", *--int_ptr);

}
