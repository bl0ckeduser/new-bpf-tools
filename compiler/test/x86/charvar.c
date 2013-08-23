main()
{
	char c = 0;
	int foo = 0;

	if (c)
		puts("fail");

	c = 65;
	foo = 123;
	putchar(c);

	c = 'B';
	foo = 123;
	putchar(c);

	c = 'B' + 1;
	foo = 123;
	putchar(c);

	printf("\n");
}
