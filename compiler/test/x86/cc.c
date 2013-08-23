trace(n) {
	printf("%d\n", n);
}

main() {
	int x = 5;
	int y = 3;

	if (x == 5 && y == 4)
		printf("A\n");

	if (x == 5 && y == 3)
		printf("B\n");

	if (x == 5 || y == 4)
		printf("C\n");

	trace(1 || 2 || 3);
	trace(0 || 2 || 3);
	trace(0 || 0 || 3);
	trace(0 || 0 || 3);
	trace(0 || 0 || 0);
	trace(3 || 0 || 0);
	trace(3 || 3 || 0);
	trace(1 || 3 || 0);

	y = 1 || ++x || 0 || ++x;
	trace(x);
	trace(y);

	x = -1;
	y = 0 || ++x || 0 || ++x;
	trace(x);
	trace(y);

	/* ============================ */

	trace(1 && 2 && 3);
	trace(0 && 2 && 3);
	trace(0 && 0 && 3);
	trace(0 && 0 && 3);
	trace(0 && 0 && 0);
	trace(3 && 0 && 0);
	trace(3 && 3 && 0);
	trace(1 && 3 && 0);

	y = 1 && ++x && 0 && ++x;
	trace(x);
	trace(y);

	y = 0 && ++x && 0 && ++x;
	trace(x);
	trace(y);
}
