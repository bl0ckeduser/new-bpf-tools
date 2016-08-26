struct s {
	int a;
	int b;
	int c;
};

int main()
{
	struct s v[10], w, x[32], y;

	v[5].a = 123;
	v[5].b = 456;
	v[5].c = 789;

	x[10].a = 11;
	x[10].b = 22;
	x[10].c = 33;

	w = v[5];
	y = x[10];

	printf("%d\n", v[5].a);
	printf("%d\n", v[5].b);
	printf("%d\n", v[5].c);

	printf("%d\n", w.a);
	printf("%d\n", w.b);
	printf("%d\n", w.c);

	printf("%d\n", x[10].a);
	printf("%d\n", x[10].b);
	printf("%d\n", x[10].c);

	printf("%d\n", y.a);
	printf("%d\n", y.b);
	printf("%d\n", y.c);


}
