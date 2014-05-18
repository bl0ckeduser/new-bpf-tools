/*
 * Big factorial
 *
 * This test file is based on program which I wrote in
 * April 2012 (at which point I was much more noobish than now)
 * that calculates factorials even if
 * they are several digits long. (maximum 1000 digits).
 *
 * The technique is somewhat inefficient and
 * stupid and really stupid in parts, for example
 * the array of carries is an awful waste since
 * only one carry variable is actually necessary.
 *
 * Anyway this should help test the compiler.
 */

zero(char *ptr, int siz)
{
	int i;
	for (i = 0; i < siz; ++i)
		*ptr++ = 0;
}

wannabe_memcpy(char *dest, char *src, int siz)
{
	int i;
	for (i = 0; i < siz; ++i)
		*dest++ = *src++;
}

main(int argc, char** argv)
{
	char bignum_1[1000];
	char bignum_2[1000];
	int i, j = 0, k = 0;

	zero(bignum_1, 1000);
	zero(bignum_2, 1000);

	bignum_1[999] = 1;

	printf("NOTE: there's a limit of 1000 digits\n");
	printf("calculate factorial of: ");
	scanf("%d", &i);

	while(i--){
		/* memcpy(bignum_2, bignum_1, 1000); */
		wannabe_memcpy(bignum_2, bignum_1, 1000);
		for(j = i; j>0; j--)
			bignum_add(bignum_1, bignum_2);
	}

	bignum_put(bignum_1);
	return 0;
}

bignum_put(char* a){
	int i = 0;
	while(!a[i]) ++i;
	while(i < 1000) putchar(a[i++] + '0');
	/* putchar('\n'); */
	printf("\n");
}

bignum_add(char* a, char* b){
	char carry[1000];
	int i = 1000 - 1;
	int x = 0;

	carry[i] = 0;

	while(i >= 0){
		carry[i - 1] = 0;
		a[i] += b[i];
		a[i] += carry[i];
		if(a[i] > 9){
			if(i > 0){
				carry[i - 1] = a[i] / 10;
				a[i] -= (char)(a[i] / 10) * 10;
			} else {
				printf("integer overflow\n");
				exit(0);
			}
		}
		i--;
	}
}


