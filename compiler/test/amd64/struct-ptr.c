typedef struct stuff {
	int a;
	int b;
	char message[16];
	int c;
} stuff_datum_t;

stuff_datum_t* foobar()
{
	stuff_datum_t *sdt = malloc(4096);
	sdt->a = 123456789;
	return sdt;
}

int derp(stuff_datum_t* ptr)
{
	ptr->b = 456;
	return ptr->a;
}

main()
{
	stuff_datum_t *sd /* = malloc(1000) */;

	sd = malloc(1000);
	sd->a = 123;
	sd->b = 456;
	sd->c = 789;

	printf("%d\n", derp(sd));
	printf("%d\n", sd->b);
	printf("%d\n", foobar()->a);
}
