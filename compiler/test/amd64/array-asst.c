main()
{
	char buf[512];
	long derp[512];
	char *ptr1, *ptr2;
	int i;
	char c_index;
	int i_index;
	char *foo = malloc(0xF);
	char *strings[32];

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
	derp[i] = buf[i] = '\0';	// null-termination

	/* test the char array writing */
	puts(buf);
	for (i = 0; derp[i]; ++i)
		putchar(derp[i]);
	printf("\n");

	/* test the int array writing */
	for (i = 0; buf[i]; ++i)
		putchar(buf[i]);
	printf("\n");

	/* test char-type array indices */
	c_index = 23;
	i_index = 23;
	printf("char-indexed: %d\n", buf[c_index]);
	printf("int-indexed: %d\n", buf[i_index]);
	printf("char-indexed: %c\n", derp[c_index]);
	printf("int-indexed: %c\n", derp[i_index]);

	/* test char dereferencing */
	ptr1 = &buf[12];
	printf("*ptr1 = '");
	putchar(*ptr1);
	printf("' \n");

	/* test casted dereference */
	*(int *)foo = 'A' + 0xFF00;
	printf("%c\n", *(char *)foo);
	printf("%d\n", *(int *)foo);

	/* test arbitrary-pointee-size pointer dereferenced assignments */
	buf[55] = 'A';
	buf[56] = 'B';
	buf[57] = 'C';
	ptr1 = &buf[56];
	*ptr1 = 'E';
	putchar(buf[55]);
	putchar(buf[56]);
	putchar(buf[57]);
	printf("\n");

	/* array pre-increments, pre-decrements */
	buf[100] = 30;
	++buf[100];
	printf("%d\n", buf[100]);
	--buf[100];
	printf("%d\n", buf[100]);
	derp[100] = 456;
	++derp[100];
	printf("%d\n", derp[100]);
	--derp[100];
	printf("%d\n", derp[100]);
	strings[14] = "abracadabra";
	++strings[14];
	puts(strings[14]);
	puts(++strings[14]);
	puts(--strings[14]);

	/* check the sizes of int and char */
	ptr1 = &buf[5];
	ptr2 = &buf[0];
	printf("%d = 5 ?\n", ptr1 - ptr2);

	ptr1 = &derp[5];
	ptr2 = &derp[0];
	printf("%d = 20 ?\n", ptr1 - ptr2);
}
