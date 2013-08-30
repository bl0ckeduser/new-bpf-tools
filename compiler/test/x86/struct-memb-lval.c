main() {
	struct foo {
		int b;
	} *x = malloc(16), y;

	y.b  = 0;
	++y.b;
	y.b++;
	++y.b;
	y.b++;
	++y.b;
	++y.b;
	++y.b;
	++y.b;
	y.b--;
	printf("%d\n", y.b);
	printf("%d\n", ++y.b);
	printf("%d\n", --y.b);
	printf("%d\n", ++y.b);
	printf("%d\n", y.b++);
	printf("%d\n", y.b++);
	printf("%d\n", --y.b);
	printf("%d\n", y.b++);
	printf("%d\n", y.b--);
	printf("%d\n", y.b--);


	x->b = 0;
	++x->b;
	++x->b;
	++x->b;
	++x->b;
	x->b++;
	++x->b;
	++x->b;
	x->b--;
	++x->b;
	printf("%d\n", ++x->b);
	printf("%d\n", ++x->b);
	printf("%d\n", ++x->b);
	printf("%d\n", x->b++);
	printf("%d\n", x->b++);
	printf("%d\n", x->b--);
	printf("%d\n", x->b--);
	printf("%d\n", --x->b);
	printf("%d\n", ++x->b);
	printf("%d\n", ++x->b);
	printf("%d\n", x->b--);
}
