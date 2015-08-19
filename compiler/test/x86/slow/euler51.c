
/* 
 * projecteuler problem 51
 * solution by: blockeduser
 * date: May 21, 2013
 *
 * Tweaked August 2013 to compile with my
 * wannabe C compiler. With my wannabe compiler,
 * it takes 2m31.562s to run on a 1.50 GHz system,
 * which is excessively slow by euler stanadrds :|
 * The original C version, compiled with gcc -O3
 * in amd64 mode, took 42.661s to run, which wasn't
 * as bad... Anyway this file tests lots of stuff.
 */

int *maxnum, *minnum;

populate()
{
	int i, maxb = 0, minb = 0;
	maxnum = malloc(32);
	minnum = malloc(32);
	for (i = 0; i < 8; ++i) {
		if (i > 1) {
			if (!minb)
				minb = 10;
			else
				minb *= 10;
		}

		if (i) {
			if (!maxb)
				maxb = 9;
			else {
				maxb *= 10;
				maxb += 9;
			}
		}

		minnum[i] = minb;
		maxnum[i] = maxb;
	}
}

fill_sieve(char *sieve, max)
{
	int n, k;
	sieve[1] = 0;
	k = 2;
	while (k < max)
		sieve[k++] = 1;

	k = 1;
	while(k++ < max){
		n = 2;
		while(n * k <= max){
		    sieve[n*k] = 0;
		    n++;
		}
	}
}

biggest_match(char *patt, len)
{
	int i;
	int res = 0;

	for (i = len - 1; i >= 0; --i) {
		if (patt[i] < 11)
			res += patt[i] - 1;
		else {
			res += 9;
		}
		res *= 10;
	}
	return res / 10;
}

smallest_match(char *patt, len)
{
	int i;
	int res = 0;

	for (i = len - 1; i >= 0; --i) {
		if (patt[i] < 11)
			res += patt[i] - 1;
		else {
			res += 0;
		}
		res *= 10;
	}
	return res / 10;
}

match(char *patt, num, len)
{
	int i = 0;
	int var = -1;

	while (num && i >= 0) {
		if (patt[i] < 11) {
			if ((num % 10) != patt[i] - 1)
				return 0;
		}
		else
		{
			if (var == -1)
				var = num % 10;
			else if (var != (num % 10))
				return 0;
		}

		++i;
		num /= 10;

		if (!num && !patt[i])
			return 1;	
	}
	
	return 0;
}

print_patt(char *patt, len)
{
	int i;
	for (i = len - 1; i >= 0; --i) {
		if (patt[i] < 11)
			putchar(patt[i] - 1 + '0');
		else {
			putchar('.');
		}
	}
}

print_matches(char *patt, char *sieve, len)
{
	int i;
	for (i = minnum[len]; i < 2000000; ++i) {
		if (i > maxnum[len])
			break;
		if (sieve[i] && match(patt, i, len)) {
			printf(" %d", i);
		}
	}
}

main()
{
	char *sieve;
	int* prime_list;
	int pc = 0;
	int start = 100;
	int end = 1000;
	int i, j, k;
	int pri;
	int dig = 3;
	char prim[256];
	char patt[256];
	int len;
	int mc;
	int fm;
	int mr = 0;
	int maxmatch;
	int minmatch;
	int mixed;
	int dc = 1;
	sieve = malloc(2000000);
	int start_offs[10];

	populate();

	for (i = 0; i < 256; ++i) {
		patt[i] = 0;
		if (i < 10)
			start_offs[i] = 0;
	}

	prime_list = malloc(2000000 * sizeof(int));

	if (!sieve) {
		printf("malloc fail\n");
		exit(1);
	}

	fill_sieve(sieve, 2000000);

	for (i = 0; i < 2000000; ++i) {
		if (sieve[i]) {
			prime_list[pc++] = i;
			if (i > maxnum[dc]) {
				dc++;
				printf("%d is the first %d-digit prime\n", i, dc);
				start_offs[dc] = pc - 1;
			}
		}
	}

	printf("----------\n");

	++*patt;
	while (patt[7] == 0) {
		for (i = 0; i <= 7; ++i) {
			if (patt[i] > 11) {
				patt[i] = 1;
				++patt[i+1];
				if (patt[i+1] == 1)
					patt[i+1] = 2;
			}
		}

		if (*patt < 11 && *patt % 2 == 1)
			++*patt;
		if (*patt < 11 && *patt - 1 == 5)
			++*patt;

		len = 0;
		mixed = 0;
		for (i = 0; i <= 7; ++i) {
			if (patt[i] != 0)
				++len;
			if (patt[i] == 11)
				mixed = 1;
		}

		if (len && mixed) {
			mc = 0;
			fm = -1;
			maxmatch = biggest_match(patt, len);
			minmatch = smallest_match(patt, len);

			if (minmatch < minnum[len])
				minmatch = minnum[len];

			for (pri = start_offs[len]; pri < start_offs[len + 1]; ++pri) {
				if (prime_list[pri] > minmatch) {
					--pri;
					break;
				}
			}

			for (i = pri; prime_list[i] <= maxmatch; ++i) {
				if (match(patt, prime_list[i], len)) {
					if (fm == -1)
						fm = prime_list[i];
					++mc;
				}
			}
			if (mc && mc > mr) {
				mr = mc;
				printf("%d: %d matches ", fm, mc);
			
				if (mc == 8) {
					printf("\nThe matches are: ");
					print_matches(patt, sieve, len);
					printf("\n");
					printf("The pattern is: ");
					print_patt(patt, len);
					printf("\n");
					goto done;
				}
				printf("\n");
			}
		}
	
		++*patt;
	}		
		
	done:
	free(sieve);
}
