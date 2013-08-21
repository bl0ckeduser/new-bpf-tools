main() {
	char buf[100];
	int i;

	scanf("%s", buf);

	/*
	 * Notice that the compiler correctly
	 * indexes into char arrays
	 */
	for (i = 0; buf[i]; ++i)
		putchar(buf[i]);

	printf("\n");
}
