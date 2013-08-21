main() {
	int i[32];
	int foo = 4;

	i[4] = 123;
// XXX: FIXME: runs out of regiters
//	printf("%d\n", *(i + foo));
	printf("%d\n", *(i + 4));
	printf("%d\n", *(4 + i));
	printf("%d\n", *(2 + 2 + i));
	printf("%d\n", *(2 + i + 2));
	printf("%d\n", *(0 + 0 + i + 1 + 1 + 2));
}
