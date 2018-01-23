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
