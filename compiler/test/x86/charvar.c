/*
 * Test operations on char variables
 */

main()
{
	int bar = 0;
	char c = 0;
	int i;
	int foo = 0;
	char ca[32];

	if (c)
		puts("fail");

	c = 65;
	foo = 123;
	bar = 456;
	putchar(c);

	c = 'B';
	foo = 123;
	bar = 456;
	putchar(c);

	c = 'B' + 1;
	foo = 123;
	bar = 456;
	putchar(c);

	printf("\n");

	/* arithmetic */

	ca[10] = 50;
	ca[11] = 60;
	ca[12] = 70;
	ca[11] = ca[11] + ca[11];
	printf("123=%d 456=%d ?\n", foo, bar);
	printf("%d %d %d\n", ca[10], ca[11], ca[12]);

	c = 5;
	c = c + c;
	printf("%d\n", c);

	i = ca[11] + ca[11];
	printf("%d\n", i);
}
