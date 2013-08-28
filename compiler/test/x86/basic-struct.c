main()
{
	struct stuff {
		int a;
		int b;
		char message[16];
		int c;
	} stuff_datum;

	stuff_datum.a = 123;
	stuff_datum.b = 456;
	strcpy(stuff_datum.message, "HELLO !!!!");
	stuff_datum.c = 789;

	printf("%d %d %d\n",
		stuff_datum.a, stuff_datum.b, stuff_datum.c);
	puts(stuff_datum.message);
}
