/*
 * fun with polynomials
 */
main()
{
	int i;
	for (i = 1; i <= 4; ++i)
		printf("%c", -12*i*i*i + 87*i*i - 186*i+217);
	printf(" ");
	for (i = 1; i <= 5; ++i)
		printf("%c",
                       (-18*i*i*i*i + 226*i*i*i - 966*i*i+1598*i -150)/6);

	printf(" ");
	for (i = 1; i <= 4; ++i)
		printf("%c",
                       (241*i*i*i-1710*i*i+3509*i-1410)/6);
	printf(" ");
	for (i = 1; i <= 4; ++i)
		printf("%c",
                       (-5*i*i*i+60*i*i-187*i+822)/6);
	printf(" ");
	for (i = 1; i <= 3; ++i)
		printf("%c",
                       (-23*i*i+95*i+122)/2);
	printf(" ");
	for (i = 1; i <= 4; ++i)
		printf("%c",
                       (-11*i*i*i+123*i*i-376*i+966)/6);
	printf("\n");
	for (i = 0; i < 10; ++i)
		printf("%c",
			-7*(i%2+1)+111);
	printf("\n");
}
