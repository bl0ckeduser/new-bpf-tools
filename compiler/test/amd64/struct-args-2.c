struct foo {
	int x;
	int y;
	int stuff[10];
};

int proc(int* thing, struct foo s)
{
	int i;
	printf("%d\n", s.x);
	printf("%d\n", s.y);
	for (i = 0; i < 10; ++i)
		printf("%d ", s.stuff[i]);
	printf("\n");
}

struct foo bump(struct foo s)
{
	s.x += 5;
	s.y *= 2;
	return s;
}

main()
{
	int i;
	struct foo thing;
	thing.x = 123;
	thing.y = 456;
	for (i = 0; i < 10; ++i)
		thing.stuff[i] = i * 2;
	proc(&i, thing);
	thing = bump(thing);
	proc(&i, thing);
}
