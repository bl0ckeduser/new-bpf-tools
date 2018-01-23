/* test nested function calls */

square(n) {
	return n * n;
}

main() {
	printf("%d\n", square(square(3) + square(4)));
}
