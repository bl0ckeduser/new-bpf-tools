/*
 * struct within a struct
 */

typedef struct {
	int foo;
	int bar;
	int baz;
} thing_t;

typedef struct {
	thing_t bob;
	int hahah;
} stuff_t;

thing_t proc()
{
	stuff_t s[32];
	s[3].bob.baz = 123;
	return s[3].bob;
}

main()
{
	thing_t s = proc();
	printf("%d\n", s.baz);
}
