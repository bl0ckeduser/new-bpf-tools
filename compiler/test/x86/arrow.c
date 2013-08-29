typedef struct stuff {
	int a;
	int b;
	char message[16];
	int c;
} stuff_datum_t;

main()
{
	stuff_datum_t *sd = malloc(1000) ;

	sd->a = 123;
	sd->b = 456;
	sd->c = 789;
	strcpy(sd->message, "hahaha");

	printf("%d%d%d\n", sd->a, sd->b, sd->c);
	puts(sd->message);
	printf(">> '%c' <<\n", sd->message[5]);
}
