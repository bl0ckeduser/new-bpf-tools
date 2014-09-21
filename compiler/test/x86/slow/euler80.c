#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Projecteuler problem 80
	by  blockeduser

  Original C version written: Oct. 13, 2012
  Tweaked to work in wannabe-compiler: Nov. 2, 2013

  The algorithm is pretty dumb, and not really described
  so well by the comments. At least it is short and seems to work.
  The program is kind of slow.

  Bugs in wannabe-compiler highlighted in the changes (XXX: ) made here:
	- no preprocessor, so no NULL
	- no unary +

  Here is the problem statement:

 "It is well known that if the square root of a natural number is not an
  integer, then it is irrational. The decimal expansion of such square
  roots is infinite without any repeating pattern at all.

  The square root of two is 1.41421356237309504880..., and the digital
  sum of the first one hundred decimal digits is 475.

  For the first one hundred natural numbers, find the total of the digital
  sums of the first one hundred decimal digits for all the irrational
  square roots." 
  (Source: http://projecteuler.net/problem=80)

	Some trial and error reveals that what is meant
	is the sum of the 100 first digits, *including*
	the integer part, in other words, we are totally
	neglecting the decimal `.'
*/

/* bigint code herein is based on my earlier
   work on e.c, dated Sept. 30, 2012 */

typedef struct bignum {
	char* dig;	/* base-10 digits */
	int length;	/* actual number length */
	int alloc;	/* bytes allocated */
} bignum_t;

/* XXX: prototypes */
/*
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
void bignum_mul(bignum_t* a, bignum_t* b);
int bignum_cmp(bignum_t* a, bignum_t* b);
int do_sqrt(int s);
*/

void require(int e, char* msg)
{
	if (!e) {
		fprintf(stderr, "failure: %s\n", msg);
		exit(1);
	}
}

bignum_t* acc;
bignum_t* one;

int main()
{
	int i;
	long s = 0;
	acc = make_bignum();
	one = make_bignum();
	bignum_nset(one, 1);

	for (i = 1; i <= 100; i++)
		s += do_sqrt(i);

	bignum_free(acc);
	bignum_free(one);

	printf("%ld\n", s);

	return 0;
}

int do_sqrt(int s)
{
	int i, j;
	int n;
	/* float x; */
	bignum_t* x = make_bignum();
	int k;
	int c;
	/* float max; */
	bignum_t* max = make_bignum();

	int ds = 0;	/* digital sum */
	int dc = 0;	/* digit count */

	int r;

	char buf[102];
	char *p = &buf[0];

	int irrational = 0;

	for (n = 1; n * n <= s; n++)
		;
	--n;

	printf("sqrt %d = %d.", s, n);

	/* add the integer part to the digital sum */
	r = n;
	while (r) {
		ds += r % 10;
		dc++;
		r /= 10;
	}	

	/* x = n * 10; */
	bignum_nset(x, n);
	bignum_shift(x);
	k = 1;
	/* max = s * pow(10, 2 * k); */
	bignum_nset(max, s);
	for (i = 0; i < 2 * k; i++)
		bignum_shift(max);


	for (k = 0; k < 101; k++) {
		c = 0;
		while (1) {
			/* if (x * x > max) { */
			bignum_copy(acc, x);
			bignum_mul(acc, x);
			if (bignum_cmp(acc, max) == -1) {
				/* --x; */
				bignum_sub(x, one);
				--c;
				break;
			}

			if (c >= 9) {
				break;
			}

			/* x++; */
			bignum_add(x, one);
			c++;
		}

		printf("%d", c);

		/* the root is irrational if, out of the
		 * 100 first decimals, we have at least
		 * one digit that isn't zero */
		irrational |= (c != 0);

		if (dc < 100) {
			ds += c;
			++dc;
		}

		if (c < 0) {
			printf("\n ERROR \n");
			break;
		}

		for (i = 0; i < 2; i++)
			bignum_shift(max);
		bignum_shift(x);
	}


	/* we don't tally the rational roots ! */
	if (!irrational)
		ds = 0;

	printf("... -> %d\n", ds);

	bignum_free(x);
	bignum_free(max);
	return ds;
}

/* ================================================ */


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
	char *new;
	char *old;

	if (bn->length == 0)
		return;

	new = malloc(bignum_len(bn) + 1);
	old = bn->dig;

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

void bignum_mul(bignum_t* a, bignum_t* b)
{
	int i, j, k;
	bignum_t *oper = make_bignum();
	bignum_t *acc = make_bignum();

	if (!oper || !acc) {
		printf("malloc failed\n");
		exit(1);
	}

	bignum_copy(oper, a);
	bignum_nset(a, 0);

	for (i = 0, j = 0; i <= b->length - 1; i++, j++) {
		bignum_copy(acc, oper);
		bignum_nmul(acc, b->dig[i]);

		trunczeroes(acc);

		for (k = 1; k <= j; k++)
			bignum_shift(acc);

		bignum_add(a, acc);
	}


	bignum_free(oper);
	bignum_free(acc);
}

/* fast multiplication by an natural
 * number that fits in an int. learnt this
 * trick from a discussion on comp.lang.c 
 * on computing factorials */
void bignum_nmul(bignum_t* a, int n){
	int max_len = a->length;
	int i, j;
	long carry = 0;
	long dig;

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

void bignum_strset(bignum_t* a, char *s){
	int i;

	/* special case for zero */
	if (*s == '0' && !s[1]) {
		a->length = 0;
		return;
	}

	if (!(strlen(s) < a->alloc)) {
		a->alloc = strlen(s);
		require((a->dig = realloc(a->dig, a->alloc)) != NULL,
			"add new digit during addition");
	}

	a->length = strlen(s);
	for (i = a->length - 1; i >= 0; i--)
		a->dig[i] = *s++ - '0';

	trunczeroes(a);
}

/* convert natural int to bignum */
void bignum_nset(bignum_t* a, int n){
	int i;

	/* special case for zero */
	if (n == 0) {
		a->length = 0;
		return;
	}

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
	long borrow = 0;

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
	long carry = 0;

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




