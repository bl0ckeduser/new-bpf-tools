/* 
 * Pointer-qualification stars (as in "int ***ptr") 
 * are eaten and IGNORED by the parser for now.
 * The language currently compiled is actually
 * typeless, but I need to write the star-qualifiers
 * in the test code so automatic comparison with
 * gcc-compiled output is possible.
 */

trace(n) {
	printf("%d\n", n);
}

main() {
	int x = 123;
	int y = 456;
	int *ptr = &y;
	int **pptr = &ptr;
	int derp[32];
	int* ptr2;

	trace(x);
	trace(*ptr);

	derp[2] = 789;
	ptr2 = &derp[2];
	trace(*ptr2);
	
	trace(*&x);
	trace(*&derp[2]);

	**pptr = 456;
	trace(y);
	
	*ptr = 1234;
	trace(y);
}
