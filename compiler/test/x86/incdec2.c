main()
{
	int i;
	int *int_ptr;
	int buf[8];

	for (i = 0; i < 8; ++i)
		buf[i] = i + i;

	/*
	 * int_ptr is a pointer to int.
	 * this means it is itself
	 * a 4-byte quantity, and
	 * that because of
	 * pointer arithmetic,
	 * pre-incrementing it
	 * increases it by exactly 4.
	 */
	int_ptr = &buf[0];
	for (i = 0; i < 5; ++i)
		printf("%d\n", *++int_ptr);
}
