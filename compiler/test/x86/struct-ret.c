/*
 * struct-type routine returns
 * supported since 2014-03-11
 */

typedef struct foo {
	int a[10];
	int b[20];
} thing;

thing routine(void) {
	thing bob;
	bob.a[0] = 123;
	return bob;
}

main()
{
	thing foo = routine();
	printf("%d\n", foo.a[0]);
}
