int bar = 12 * 34 + 56;

main()
{
	int foo[100 * 100 + 10];
	foo[100 * 100 + 5] = 123;
	foo[100 * 100 + 6] = 456;
	printf("%d%d\n",
		foo[10005], foo[10006]);
	printf("%d\n", bar);
}
