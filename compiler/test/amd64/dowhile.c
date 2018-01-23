main()
{
	int x;

	x = 6;
	do {
		printf("%d", x--);
		if (x == 1)
			break;
		if (x == 3)
			continue;
		printf("%d", x);
	} while(--x);
	printf("\n");

	do printf("ha"); while(++x < 3);
	printf("\n");
}
