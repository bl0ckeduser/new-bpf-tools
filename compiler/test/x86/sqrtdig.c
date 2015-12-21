#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* extract a root to N decimals

	by blockeduser
	  ^	^   ^
	  |	|   |
	 therefore a better algorithm exists

   well yeah this is a rather inefficient naive algorithm

   for testing pruproses this has been hardwired to do
   sqrt(2) to 50 places

   originally written in real C October 12, 2012.
   (as always some little tweaks needed to feed it into
   the wannabe-compiler)

   scientific notice: Newton's method would probably be
   better on this one. or maybe taylor series? god knows,
   and i don't even know if god exists, so let's end this 
   here.
 */

long null = 0x0;

/* bigint code herein is based on my earlier
   work on e.c, dated Sept. 30, 2012 */

typedef struct bignum {
	char* dig;	/* base-10 digits */
	int length;	/* actual number length */
	int alloc;	/* bytes allocated */
} bignum_t;

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

bignum_t* make_bignum()
{
	bignum_t* bn;
	require((bn = malloc(sizeof(bignum_t))) != null, 
		"bignum structure allocation");
	require((bn->dig = malloc(1000)) != null, 
		"bignum dig allocation");
	bn->alloc = 1000;
	bn->length = 0;
	return bn;
}


void require(int e, char* msg)
{
	if (!e) {
		fprintf(stderr, "failure: %s\n", msg);
		exit(1);
	}
}

int main(int argc, char** argv)
{
	int i;
	int s;
	int n;
	/* float x; */
	bignum_t* x = make_bignum();
	int k;
	int c;
	/* float max; */
	bignum_t* max = make_bignum();
	int d;

	bignum_t* acc = make_bignum();
	bignum_t* one = make_bignum();

	/*
	if (argc != 3) {
		printf("%s number digits\n", *argv);
		return 1;
	}
	*/

	sscanf("2", "%d", &s);
	sscanf("50", "%d", &d);

	bignum_nset(one, 1);

	for (n = 1; n * n <= s; n++)
		;
	--n;

	/* x = n * 10; */
	bignum_nset(x, n);
	bignum_shift(x);
	k = 1;
	/* max = s * pow(10, 2 * k); */
	bignum_nset(max, s);
	for (i = 0; i < 2 * k; i++)
		bignum_shift(max);


	printf("sqrt %d = ", s);
	printf("%d.", n);

	for (k = 0; k < d; k++) {

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

			if (c >= 9)
				break;

			/* x++; */
			bignum_add(x, one);
			c++;
		}

		printf("%d", c);

		if (c < 0) {
			printf("\n ERROR \n");
			break;
		}

		for (i = 0; i < 2; i++)
			bignum_shift(max);
		bignum_shift(x);
	}

	printf("...\n");

	bignum_free(x);
	bignum_free(max);
	bignum_free(acc);
	bignum_free(one);
	return 0;
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
		require(dest->dig != null, "resize on copy");
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
			require((a->dig = realloc(a->dig, a->alloc)) != null,
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

	/* special case for zero */
	if (n == 0) {
		a->length = 0;
		return;
	}

	for (i = 0; n; i++, n /= 10) {
		if (!(i < a->alloc)) {
			a->alloc += 5;
			require((a->dig = realloc(a->dig, a->alloc)) != null,
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
			require((a->dig = realloc(a->dig, a->alloc)) != null,
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
			require((a->dig = realloc(a->dig, a->alloc)) != null,
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




