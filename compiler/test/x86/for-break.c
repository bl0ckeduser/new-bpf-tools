/*
 * Test "break;" statement
 * in while and for loops
 */

main()
{
	int i, j, k;

	for (;;)
		break;

	for (i = 0; i < 10; ++i)
		if (i > 3)
			break;
	printf("i=%d\n", i);

	k = 5;
	while (--k)
		if (k == 2)
			break;
	printf("k=%d\n", k);

	for(i = 0; i < 3; ++i) {
		for (j = 0; j < 5; ++j) {
			printf("i=%d, j=%d\n", i, j);
			if (j > 3)
				/* this should stop the
				 * inner loop only ! */
				break;
		}
	}
}
