/*
 * struct-type routine returns
 * not supported yet
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
