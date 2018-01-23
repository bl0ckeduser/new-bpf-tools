main()
{
	struct {
		int a;
		int b;
		int c;
		int d;
	} foo[32], *bar;

	foo[5].c = 123;
	bar = foo;
	printf("%d\n", bar[5].c);
}
