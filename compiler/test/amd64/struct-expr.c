/*
 * nasty struct expressions with
 * lots of arrows and [] mixed in
 * Wed Mar 12 10:48:50 EDT 2014
 */

typedef struct derp {
	struct derp *child[100];
	struct derp *tok;
	int start;
} thing;

main()
{
	thing *b = malloc(sizeof(thing));
	b->child[0] = malloc(sizeof(thing));
	b->child[0]->tok = malloc(sizeof(thing));
	b->child[0]->tok->start = 123;
	b->start = 456;

	b->child[0]->tok->child[0] = malloc(sizeof(thing));
	b->child[0]->tok->child[0]->tok = malloc(sizeof(thing));
	b->child[0]->tok->child[0]->tok->start = 789;

	printf("%d\n", b->child[0]->tok->start);
	printf("%d\n", b->start);
	printf("%d\n", b->child[0]->tok->child[0]->tok->start);
}
