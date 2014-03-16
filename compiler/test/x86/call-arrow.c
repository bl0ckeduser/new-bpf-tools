/*
 * applying -> and . operators
 * on a routine call
 */

typedef struct {
	int a;
	int stuff[100];
	int b;
} thing;

int getb(thing t)
{
	printf("b: %d\n", t.b);
	return t.b;
}

thing routine()
{
	thing u;
	u.stuff[10] = 123;
	u.b = 456;
	return u;
}

thing* routine2()
{
	thing *ptr = malloc(sizeof(thing));
	ptr->b = 789;
	return ptr;
}

main()
{
	thing t;
	printf("%d\n", routine().stuff[10]);
	printf("%d\n", routine().b);
	printf("%d\n", getb(routine()));
	printf("%d\n", (t = routine()).b);
	printf("%d\n", getb((t = routine())));
	printf("%d\n", t.b);
	printf("%d\n", routine2()->b);
}
