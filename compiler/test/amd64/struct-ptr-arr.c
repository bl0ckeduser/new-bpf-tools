typedef struct stuff {
	int a;
	int b;
	char message[16];
	int c;
} stuff_datum_t;

main()
{
	int i;
	stuff_datum_t **ptr;
	stuff_datum_t sd;

	ptr = malloc(10 * sizeof(stuff_datum_t *));

	for (i = 0; i < 10; ++i)
		ptr[i] = malloc(sizeof(stuff_datum_t));

	for (i = 0; i < 10; ++i)
		ptr[i]->a = i;

	for (i = 0; i < 10; ++i)
		printf("%d\n", ptr[i]->a);

	/* XXX: struct assignments TODO */
	/* sd = ptr[3]; */
	/* printf("%d\n", sd.a); */
}
