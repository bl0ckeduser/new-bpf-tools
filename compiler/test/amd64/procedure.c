/* typical recursive factorial example */
factorial(n) {
	if (n > 0)
		return n * factorial(n - 1);
	else
		return 1;
}

main() {
	printf("%d\n", factorial(5));
}
