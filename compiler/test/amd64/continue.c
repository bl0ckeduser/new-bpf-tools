main()
{
	int i, j, k;
	i = 9;
	while (i > 0) {
		printf("%d", --i);
		if (i % 2 == 0)
			continue;
		printf("!");
	}
	printf("\n");
	for (i = 0; i < 5; ++i) {
		for (j = 10; j < 20; ++j) {
			if (j != 15)
				continue;
			printf("j=%d\n", j);
			for (k = 0; k < 4; ++k) {
				if (k != 3)
					continue;
				printf("%d\n", k);
			}
		}
		if (i == 3)
			continue;
		printf("i=%d\n", i);
	}
}
