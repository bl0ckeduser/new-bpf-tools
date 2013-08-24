/* test tail-call optimization */

diff(a, b) {
	if ( b > 0 )
		return diff(a - 1, b - 1);
	else
		return a;
}

main() {
	printf("%d\n", diff(10000000, 15));
}
