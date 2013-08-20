// int x = 456;
// BUG: globals outside main() are currently broken

bob() {
	int x = 123;
	printf("%d\n", x);
}

main() {
	int x = 456;
	bob();
	printf("%d\n", x);
}
