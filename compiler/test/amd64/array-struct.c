struct s {
	int a[27];
	int b[63];
};

main()
{
	struct s donaldo;
	donaldo.a[2] = 123;
	donaldo.a[10] = 456;
	printf("%d\n", donaldo.a[2]);
	printf("%d\n", donaldo.a[10]);
}
