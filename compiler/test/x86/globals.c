int ia[237];
char ca[3493];
int *ipa[49];

main()
{
	int i;
	/*
	 * Test global arrays
	 */
	printf("int ");
	for (i = 230; i < 237; ++i) {
		ia[i] = (i - 230) * (i - 230);
		ipa[i - 230] = &ia[i];
	}
	for (i = 230; i < 237; ++i)
		printf("%d ", ia[i]);
	printf("\n");
	printf("int * ");
	for (i = 0; i < 7; ++i)
		printf("%d ", *ipa[i]);
	printf("\n");
	printf("char ");
	for (i = 3435; i < 3493; ++i)
		ca[i] = (i * i) % 26 + 'A';
	for (i = 3435; i < 3493; ++i)
		putchar(ca[i]);
	printf("\n");
}

