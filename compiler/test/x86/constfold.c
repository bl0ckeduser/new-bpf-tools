int bar = 12 * 34 + 56;

main()
{
	int foo[10 * 10 + 10];
	foo[10 * 10 + 5] = 123;
	foo[10 * 10 + 6] = 456;
	printf("%d%d\n",
		foo[105], foo[106]);
	printf("%d\n", bar);
}
