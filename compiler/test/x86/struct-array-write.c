struct s {
	char hahaha[32];
	int bar;
	int foo;
	int baz;
};

main()
{
	struct s arr[32];
	struct s thing;
	struct s other_thing;

	thing.foo = 123;
	other_thing.hahaha[7] = '!';
	arr[10] = thing;
	arr[20] = other_thing;
	printf("%d\n", arr[10].foo);
	printf("%c\n", arr[20].hahaha[7]);
}
