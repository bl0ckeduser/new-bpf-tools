/*
 * A test for compiling switch statements
 * Fri Feb  7 14:28:08 EST 2014
 */

foo(u) {
	switch(u) {
		case 324:
			printf("yes\n");
			break;
		case 54:
			printf("is ");
		case 76:
			printf("dog\n");
			break;
		case 25:
			printf("hahaha\n");
			break;
		case 120:
			printf("hello\n");
			break;
		case 29:
			printf("this ");
			break;
	}
}

main() {
	int i;
	int ind[] = {324, 120, 25, 29, 54};
	for (i = 0; i < 5; ++i)
		foo(ind[i]);
}
