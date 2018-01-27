typedef struct {
	int a;
	char b;
	int c;
} thing_t;

func(char x, thing_t t, char y)
{
	printf("%c %c %c\n", t.b, x, y);
}

main()
{
	thing_t stuff[100];
	char *ptr;
	int i;
	stuff[10].a = 123;
	stuff[10].b = 'A';
	stuff[10].c = 456;
	ptr = &(stuff[10].b);
	for (i = 0; i < 100; ++i)
		if (i != 10)
			stuff[i].a = stuff[i].b = stuff[i].c = 'X';
	printf("%c=A? %d%d=123456?\n",
		*ptr, stuff[10].a, stuff[10].c);
	func('B', stuff[10], 'C');
}
