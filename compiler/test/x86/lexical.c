int x = 789;		// global

foo() {
	int x = 123;	// local
	bar();
	printf("%d\n", x);
}

bar() {
	int x = 456;	// local
	printf("%d\n", x);
}

main() {
	foo();
	printf("%d\n", x);
}
