/*
 * This is a program that will print 1000 decimal places
 * of the contant "e". It is based on the program "e.c"
 * which is a real C program I (blockeduser) wrote on Sept. 30, 2012.
 * The original program has some more comments (including
 * some on the algorithm used and the log of a performance test)
 * and a feature to choose the number of digits to be printed.
 * The original program is available at:
 * http://sites.google.com/site/bl0ckeduserssoftware/e.c
 *
 * Checksum of the 1000 places:
 * 	$ ./compile-run-x86.sh e.c 2>/dev/null | sed 's/e = 2\.//g'
 *		| sed 's/\.\.\.\(.*\)//g' | tr -d '\n' | md5sum
 *	509a0539a67baea4730a85ad0d12c289  -
 *	$
 * (Verified against http://www.greatplay.net/uselessia/articles/e2-1000.html)
 */

char *my_realloc(char *ptr, int len)
{
	char *new_ptr = malloc(len);
	int i;
	for (i = 0; i < len; ++i)
		new_ptr[i] = ptr[i];
	free(ptr);
	return new_ptr;
}

max(a, b) {
	if (a > b)
		return a;
	return b;
}

int wannabe_null = 0x0;

typedef struct bignum {
	char* dig;	/* base-10 digits */
	int length;	/* actual number length */
	int alloc;	/* bytes allocated */
} bignum_t;

bignum_t* make_bignum()
{
	bignum_t* bn;
	require((bn = malloc(sizeof(bignum_t))) != wannabe_null, 
		"bignum structure allocation");
	require((bn->dig = malloc(1000)) != wannabe_null, 
		"bignum dig allocation");
	bn->alloc = 1000;
	bn->length = 0;
	return bn;
}

require(int e, char* msg)
{
	if (!e) {
		printf("failure: %s\n", msg);
		exit(1);
	}
}

int bruteforce_terms(int digits)
{
	bignum_t* a;
	a = make_bignum();
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
	int places;
	bignum_t* ptab[30];
	bignum_t* one;

	int n, d;
	int i, j, k;

	/* for convenience of whole program */
	one = make_bignum();
	bignum_nset(one, 1);

	n = bruteforce_terms(1000);
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

	/* set up product table */
	bignum_copy(acc2, acc);
	for (i = 0; i < 9; i++) {
		ptab[i] = make_bignum();
		bignum_copy(ptab[i], acc2);
		bignum_add(acc2, acc);
	}

	printf("e = 2.");

	bignum_shift(sum);
	for (i = 0; i < places; i++) {
		for (j = 0; j < 9; j++)
			if (bignum_cmp(sum, ptab[j]) == 1)
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

trunczeroes(bignum_t* a) {
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
	char* new = malloc(bignum_len(bn) + 1);
	char* old = bn->dig;
	require(new != wannabe_null, "shift digits");
	*new = 0;
	memcpy(new + 1, old, bignum_len(bn));
	bn->dig = new;
	++bn->length;
	//free(old);
}

void bignum_free(bignum_t* bn)
{
}

void bignum_copy(bignum_t* dest, bignum_t* src)
{
	if (src->alloc > dest->alloc) {
		dest->dig = my_realloc(dest->dig, src->alloc);
		dest->alloc = src->alloc;
		require(dest->dig != wannabe_null, "resize on copy");
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
	 int carry = 0;
	 int dig;

	for(i = 0; i < max_len || carry; i++) {
		/* make space for new digits if necessary */
		if (!(i < a->alloc)) {
			a->alloc += 5;
			require((a->dig = my_realloc(a->dig, a->alloc)) != wannabe_null,
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
	for (i = 0; n; i++) {
		if (!(i < a->alloc)) {
			a->alloc += 5;
			require((a->dig = my_realloc(a->dig, a->alloc)) != wannabe_null,
				"add new digit during addition");
		}
		a->dig[i] = n % 10;
		++a->length;
		n /= 10;
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
		return 1;
	for (i = a->length - 1; i >= 0; i--) {
		if (a->dig[i] > b->dig[i])
			return -1;
		else if (a->dig[i] < b->dig[i])
			return 1;
	}
	return 0;
}

void bignum_sub(bignum_t* a, bignum_t* b){
	int max_len = max(a->length, b->length);
	int i, j;
	 int borrow = 0;

	/* negative ? */
	if (bignum_cmp(a, b) == 1) {
		printf("I don't do negatives yet, sorry.\n");
		exit(1);
	}

	/* zero ? */
	if (bignum_cmp(a, b) == 0) {
		*(a->dig) = 0;
		a->length = 1;
		return 0;
	}

	for(i = 0; i < max_len || borrow; i++) {
		/* make space for new digits if necessary */
		if (!(i < a->alloc)) {
			a->alloc += 5;
			require((a->dig = my_realloc(a->dig, a->alloc)) != wannabe_null,
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
	int max_len = max(a->length, b->length);
	int i, j;
	 int carry = 0;

	for(i = 0; i < max_len || carry; i++) {
		/* make space for new digits if necessary */
		if (!(i < a->alloc)) {
			a->alloc += 5;
			require((a->dig = my_realloc(a->dig, a->alloc)) != wannabe_null,
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


