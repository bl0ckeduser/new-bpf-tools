extern struct {
	int foo;
	int bar;
} bob;

main()
{
	init();
	printf("%d\n", (&bob)->foo);
	printf("%d\n", (&bob)->bar);
}
