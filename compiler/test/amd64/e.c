/*
 * This is a test that involves running a programming that calculates
 * the constant \e  to some number of digits, using an old program called e.c
 * dated 2012-09-30.
 * Now is some hacks to make it work with wannabe CC and to hard-wire
 * the d-value  (number of digits). After the dashes is the original program
 * preserved as-is by my trusty hard-drive, which is still fucking magnetic
 * because I was raised to be very cheap by my parents and I have not spent
 * on an SSD. Money is always hard-earned when you do real work and then
 * nearly half of it goes to the government, so don't fucking splurge!!!
 */

#define unsigned /**/

int main(int argc, char **argv)
{
	char *my_argv[] = {"foobar.elf", "-d", "1000"};
	old_main(3, my_argv);
}

#define main old_main

/* -------------------------------------------------------------------- */

/* 
 * This program gives either "N - 2" terms of the 
 * series for e, or "N" decimal places of e
 * (in the latter case converting the digit count
 * to a term count by brute-force) using only C's
 * built-in integer arithmetic (lots of it and lots
 * of memory allocation and moving-around stuff).
 * In both cases, it makes sure to stop giving
 * you digits once the error margin is reached.
 * 
 * No legal absolute guarantee of accuracy is 
 * made because this is just a hobby project.
 *
 * 	IMPORTANT:
 * The *term* count (which is not necessarily the
 * same as the decimal place count) has to fit in
 * your system's int type. I could have written 
 * this program so that it accepts arbitrarily big 
 * integers ("bignums" as I call them herein) for 
 * the term count, but accepting only ints (or some 
 * other built-in type) allows for much greater 
 * multiplication performance. (See the bignum_nmul
 * routine).
 *
 * The calculus algorithms for estimating e and 
 * estimating the error in the estimate are the 
 * same as in my earlier program e-estim.py. 
 * (MacLaurin series 1/n! and Lagrange Remainder Term
 * and some tricks. The old program has the details 
 * in its comments). The bignum arithmetic operations are
 * done "pencil-and-paper" style as I was
 * taught in elementary. The main new thing in
 * this program is the "product table" to speed
 * up division.
 *
 * Performance test (done on a 1.50 GHz, dual-core 
 * "Intel(R) Celeron(R) B800" running 64-bit Linux):
 * $ time ./e -d 200000 >test-200000.txt
 * 
 * real	15m25.528s
 * user	15m16.001s
 * sys	0m1.256s
 * 
 * During this test, the program computed
 * 200002 decimal places of e. From my tests,
 * they appeared to match the 200002 first 
 * places of NASA's "e.2mil" file (which gives 2 
 * million places).
 *
 * Memory usage test (same machine):
 * $ valgrind ./e -d 3000 2>&1 1>/dev/null | tail -8 | head -3
 * ==14427== HEAP SUMMARY:
 * ==14427==     in use at exit: 0 bytes in 0 blocks
 * ==14427==   total heap usage: 4,645 allocs, 4,645 frees, 12,278,936 bytes allocated
 * 
 * (I used far fewer digits for this test because 
 * valgrind seems to slow things down and I'm lazy).
 *
 * blockeduser Sept. 30, 2012
 */

/*
 *  NOTE: binary might be faster than
 *  base-10 for my bignums, but would
 *  lead to the problem of converting the
 *  end-result to decimal ;_;
 *  I don't know that sort of stuff well.
 *  BCD would be more memory-efficient, but
 *  also somewhat trickier.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct bignum {
	char* dig;	/* base-10 digits */
	int length;	/* actual number length */
	int alloc;	/* bytes allocated */
} bignum_t;

void require(int e, char* msg)
{
	if (!e) {
		fprintf(stderr, "failure: %s\n", msg);
		exit(1);
	}
}

bignum_t* make_bignum(void);
void bignum_copy(bignum_t* dest, bignum_t* src);
void bignum_add(bignum_t*, bignum_t*);
void bignum_sub(bignum_t*, bignum_t*);
void bignum_put(bignum_t*);
void bignum_free(bignum_t* bn);
void bignum_nmul(bignum_t* a, int n);
void bignum_nset(bignum_t* a, int n);
int bignum_len(bignum_t* a);
void bignum_shift(bignum_t* bn);

bignum_t* one;

int bruteforce_terms(int digits)
{
	bignum_t* a = make_bignum();
	int i;

	bignum_nset(a, 24);

	for (i = 5; bignum_len(a) < digits; i++) {
		require(i > 0, "lack of integer overflow");
		if (i != 3)
			bignum_nmul(a, i);
	}
	bignum_free(a);
	return i;
}

int main(int argc, char** argv)
{
	bignum_t* mul = make_bignum();
	bignum_t* acc = make_bignum();
	bignum_t* acc2 = make_bignum();
	bignum_t* sum = make_bignum();
	unsigned int places;
	bignum_t* ptab[10];

	int n, d;
	int i, j, k;

	/* for convenience of whole program */
	one = make_bignum();
	bignum_nset(one, 1);

	if (argc < 2){
		printf("use: %s <terms>\nor %s -d <decimal-places> \n", argv[0], argv[0]);
		exit(0);
	}

	if (!strcmp(argv[1], "-d")) {
		require(argc > 2, "-d argument");
		/* 
		 * very sneaky bug here where %ld must be used in lieu of %d
	 	 * if we are using 64-bit int
		 */
		sscanf(argv[2], "%ld", &d);
		n = bruteforce_terms(d);
	} else {
		sscanf(argv[1], "%ld", &n);
	}
	require(n > 3, "n > 3");

	bignum_nset(sum, 1);
	bignum_nset(acc, 1);
	bignum_nset(acc2, 1);
	bignum_nset(mul, n);

	for (i = n; i > 2; i--) {
		bignum_nmul(acc, i);
		if (i != n && i != 3)
			bignum_nmul(acc2, i);
		bignum_add(sum, acc);
	}
	bignum_nmul(acc, 2);
	bignum_nmul(acc2, 2);
	places = bignum_len(acc2);	/* decimal places of accuracy */

	/* digit-cranking starts now */
	printf("%d decimal places\n", places);
	printf("e = 2.");

	/* set up product table */
	bignum_copy(acc2, acc);	
	for (i = 0; i < 9; i++) {
		ptab[i] = make_bignum();
		bignum_copy(ptab[i], acc2);
		bignum_add(acc2, acc);
	}

	bignum_shift(sum);
	for (i = 0; i < places; i++) {
		for (j = 0; j < 9; j++)
			if (bignum_cmp(sum, ptab[j]) == +1)
				break;
		printf("%d", j);
		if (j > 0)
			bignum_sub(sum, ptab[j - 1]);
		bignum_shift(sum);	
	}

	printf("...\n");

	/* clean up */
	bignum_free(mul);
	bignum_free(acc);
	bignum_free(acc2);
	bignum_free(sum);
	bignum_free(one);

	for (i = 0; i < 9; i++)
		bignum_free(ptab[i]);

	return 0;
}

void trunczeroes(bignum_t* a) {
	int i;
	int c = 0;
 
	/* count useless leading zeroes */
	for (i = a->length - 1; i >= 0 && !a->dig[i]; i--)
		++c;

	/* take them off the length number */
	a->length -= c;
}

bignum_t* make_bignum()
{
	bignum_t* bn;
	require((bn = malloc(sizeof(bignum_t))) != NULL, 
		"bignum structure allocation");
	require((bn->dig = malloc(1000)) != NULL, 
		"bignum dig allocation");
	bn->alloc = 1000;
	bn->length = 0;
	return bn;
}

/* multiply by ten */
void bignum_shift(bignum_t* bn)
{
	char* new = malloc(bignum_len(bn) + 1);
	char* old = bn->dig;
	require(new != NULL, "shift digits");
	*new = 0;
	memcpy(new + 1, old, bignum_len(bn));
	bn->dig = new;
	++bn->length;
	free(old);
}

void bignum_free(bignum_t* bn)
{
	if (bn) {
		if (bn->dig)
			free(bn->dig);
		free(bn);
	}
}

void bignum_copy(bignum_t* dest, bignum_t* src)
{
	if (src->alloc > dest->alloc) {
		dest->dig = realloc(dest->dig, src->alloc);
		dest->alloc = src->alloc;
		require(dest->dig != NULL, "resize on copy");
	}
	memcpy(dest->dig, src->dig, src->length);
	dest->length = src->length;
}

void bignum_put(bignum_t* a){
	int i;

	for (i = a->length - 1; i >= 0; i--)
		putchar(a->dig[i] + '0');
}

int bignum_len(bignum_t* a){
	int i;
	int l = 0;

	for (i = a->length - 1; i >= 0; i--)
		++l;

	return l;
}

/* fast multiplication by an natural
 * number that fits in an int. learnt this
 * trick from a discussion on comp.lang.c 
 * on computing factorials */
void bignum_nmul(bignum_t* a, int n){
	int max_len = a->length;
	int i, j;
	unsigned long carry = 0;
	unsigned long dig;

	for(i = 0; i < max_len || carry; i++) {
		/* make space for new digits if necessary */
		if (!(i < a->alloc)) {
			a->alloc += 5;
			require((a->dig = realloc(a->dig, a->alloc)) != NULL,
				"add new digit during addition");
		}
		if (!(i < a->length)) {
			++a->length;
			a->dig[i] = 0;
		}

		dig = a->dig[i] * n;
		dig += carry;

		/* carry for the next digit */
		carry = 0;
		if (dig > 9) {
			carry = dig / 10;
			dig %= 10;
		}

		a->dig[i] = dig;
	}
	trunczeroes(a);
}

/* convert natural int to bignum */
void bignum_nset(bignum_t* a, int n){
	int i;
	for (i = 0; n; i++, n /= 10) {
		if (!(i < a->alloc)) {
			a->alloc += 5;
			require((a->dig = realloc(a->dig, a->alloc)) != NULL,
				"add new digit during addition");
		}
		a->dig[i] = n % 10;
		a->length++;
	}
	trunczeroes(a);
}

/* a > b  --> -1
 * a < b  --> + 1
 * a == b --> 0
 */
int bignum_cmp(bignum_t* a, bignum_t* b){
	int i;

	if (a->length > b->length)
		return -1;
	if (b->length > a->length)
		return +1;
	for (i = a->length - 1; i >= 0; i--) {
		if (a->dig[i] > b->dig[i])
			return -1;
		else if (a->dig[i] < b->dig[i])
			return +1;
	}
	return 0;
}

void bignum_sub(bignum_t* a, bignum_t* b){
	int max_len = a->length > b->length ? a->length : b->length;
	int i, j;
	unsigned long borrow = 0;

	/* negative ? */
	if (bignum_cmp(a, b) == +1) {
		printf("I don't do negatives yet, sorry.\n");
		exit(1);
	}

	/* zero ? */
	if (bignum_cmp(a, b) == 0) {
		*(a->dig) = 0;
		a->length = 1;
		return;
	}

	for(i = 0; i < max_len || borrow; i++) {
		/* make space for new digits if necessary */
		if (!(i < a->alloc)) {
			a->alloc += 5;
			require((a->dig = realloc(a->dig, a->alloc)) != NULL,
				"add new digit during addition");
		}
		if (!(i < a->length)) {
			++a->length;
			a->dig[i] = 0;
		}

		/* subtract the two corresponding digits
		 * if they exist and subtract the borrow */
		if (i < b->length)
			a->dig[i] -= b->dig[i];
		a->dig[i] -= borrow;

		/* borrow for the next digit */
		borrow = 0;
		while (a->dig[i] < 0) {
			a->dig[i] += 10;
			++borrow;
		}
	}
	trunczeroes(a);
}

void bignum_add(bignum_t* a, bignum_t* b){
	int max_len = a->length > b->length ? a->length : b->length;
	int i, j;
	unsigned long carry = 0;

	for(i = 0; i < max_len || carry; i++) {
		/* make space for new digits if necessary */
		if (!(i < a->alloc)) {
			a->alloc += 5;
			require((a->dig = realloc(a->dig, a->alloc)) != NULL,
				"add new digit during addition");
		}
		if (!(i < a->length)) {
			++a->length;
			a->dig[i] = 0;
		}

		/* add the two corresponding digits
		 * if they exist and add the carry */
		if (i < b->length)
			a->dig[i] += b->dig[i];
		a->dig[i] += carry;

		/* carry for the next digit */
		carry = 0;
		if (a->dig[i] > 9) {
			carry = a->dig[i] / 10;
			a->dig[i] %= 10;
		}
	}
	trunczeroes(a);
}



