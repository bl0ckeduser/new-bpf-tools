main()
{
	int i;
	char *char_ptr;
	char buf[8];

	sprintf(buf, "hello");
	puts(buf);

	/*
	 * char_ptr is a pointer to char.
	 * this means it is itself
	 * a 4-byte quantity, and
	 * that because of
	 * pointer arithmetic,
	 * pre-incrementing it
	 * increases it by exactly 1.
	 */
	char_ptr = &buf[0];
	for (i = 0; i < strlen(buf); ++i)
		puts(++char_ptr);
}
