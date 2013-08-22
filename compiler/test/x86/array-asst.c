main()
{
	char buf[512];
	int derp[512];
	char *ptr1, *ptr2;
	int i;

	for (i = 0; i < 7 * 10; ++i)
		if (i % 7 == 6) {
			/*
			 * the compiler correctly writes
			 * into char arrays
			 */
			buf[i] = ' ';

			/* and into int arrays */
			derp[i] = buf[i];
		}
		else {
			/* random amusing pattern ... */
			buf[i] = 'a' + (i % 7);
			derp[i] = buf[i];
		}
	derp[i] = buf[i] = 0;	// null-termination

	/* test the char array writing */
	puts(buf);
	for (i = 0; derp[i]; ++i)
		putchar(derp[i]);
	printf("\n");

	/* test the int array writing */
	for (i = 0; buf[i]; ++i)
		putchar(buf[i]);
	printf("\n");

	/* check the sizes of int and char */

	ptr1 = &buf[5];
	ptr2 = &buf[0];
	printf("%d = 5 ?\n", ptr1 - ptr2);

	ptr1 = &derp[5];
	ptr2 = &derp[0];
	printf("%d = 20 ?\n", ptr1 - ptr2);
}
