main() {
	char buf[100];
	char *ptr;
	int i;

	printf("please type a string: ");
	scanf("%s", buf);

	/*
	 * Notice that the compiler correctly
	 * indexes into char arrays
	 */
	printf("full string: ");
	for (i = 0; buf[i]; ++i)
		putchar(buf[i]);
	printf("\n");

	/*
	 * char addresses are also
	 * properly done
	 */
	printf("string, with 3 first chars omitted: ");
	ptr = &buf[3];
	puts(ptr);

}
