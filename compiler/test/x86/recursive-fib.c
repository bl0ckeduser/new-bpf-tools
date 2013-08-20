fib(n) {
	if (n < 2)
		return n;
	else
		return fib(n - 1) + fib(n - 2);
}

main() {
	int t = 0;
	while(t < 20)
		printf("%d\n", fib(t++));
}
