typedef struct {
	int foo;
	int bar;
} node_t;

typedef struct {
	int alloc;
	int pointer;
	node_t *array;
} array_stack;

array_stack* make_array_stack()
{
	array_stack *as = malloc(sizeof(array_stack));
	as->alloc = 3;
	as->pointer = 0;
	as->array = malloc(as->alloc * sizeof(node_t));
	return as;
}

void push(array_stack* l, node_t thing)
{
	++l->pointer;
	if (l->pointer >= l->alloc) {
		l->alloc *= 2;
		l->array = realloc(l->array, (l->alloc)*sizeof(node_t));
	}
	l->array[l->pointer - 1] = thing;
}

node_t pop(array_stack* l)
{
	if (l->pointer == 0) {
		printf("nothing to fucking pop\n");
		exit(1);
	}
	return l->array[--l->pointer];
}

main()
{
	array_stack *as = make_array_stack();
	node_t t;
	int i;
	for (i = 0; i < 20; ++i) {
		t.foo = i;
		push(as, t);
	}
	for (i = 0; i < 20; ++i) {
		t = pop(as);
		printf("%d\n", t.foo);
	}
}
