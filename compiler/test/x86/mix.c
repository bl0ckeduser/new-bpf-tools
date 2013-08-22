/*
 * This program tests that the
 * compiler succesfully deals
 * with adjacent variables
 * of various types and sizes
 * and succesfully writes and
 * reads from them.
 */

main()
{
	int i = 123;
	char *ptr;
	char **pptr;
	char c = 'A';
	int j = 923493;
	char buf[5];
	int k;

	ptr = &buf[0];
	pptr = &ptr;
	sprintf(buf, "ABCD");
	k = 923493;
	j = 2147483647;

	puts(buf);
	putchar(c);
	printf("\n");
	printf("%d\n", i);
	printf("%d\n", j);
	printf("%d\n", k);
	puts(*pptr);

	c = 0;
	i = 1943;
	j = 2395;
	if (c)
		puts("fail");

	for (i = 0; i < 'A'; ++i)
		c++;
	i = 1943;
	j = 2395;
	putchar(++c);
	printf("\n");
}
