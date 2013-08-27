typedef int dinosaur;
typedef struct bignum {
	char* dig;	/* base-10 digits */
	int length;	/* actual number length */
	int alloc;	/* bytes allocated */
} bignum_t;

print(dinosaur d) {
	/* now you know where the %d in printf came from.
	 * don't tell anybody, now. */
	printf("%d\n", d);
}

main()
{
	dinosaur donald;
	bignum_t bob, derp;

	bob.length = 123;
	derp.length = 456;
	donald = 789;
	printf("%d\n", bob.length);
	printf("%d\n", derp.length);
	printf("%d\n", donald);
}
