/* test pointer arithmetic */

main() {
	int i[32];
	int foo = 4;
	int *ptr = i + foo;
	char **bob;
	char **bob2;
	char **bob3;
	int *j = &i[6];

	i[3] = 456;
	i[4] = 123;
	printf("%d\n", *(i + foo));
	printf("%d\n", *(i + foo - 1));
	printf("%d\n", *(j - 2));
	printf("%d\n", *(j - 1 - 1));
	printf("%d\n", *ptr);
	printf("%d\n", *(i + 4));
	printf("%d\n", *(4 + i));
	printf("%d\n", *(2 + 2 + i));
	printf("%d\n", *(2 + i + 2));
	printf("%d\n", *(0 + 0 + i + 1 + 1 + 2));

	/*
	 * this should print "DERP" because
	 * the increment should move bob to
	 * its next index
	 */
	bob = malloc(1024);
	bob[0] = "HERP";
	bob[1] = "DERP";
	bob[2] = "HAX";
	puts(*++bob);

	bob3 = &bob[1];
	puts(*bob3);
}
