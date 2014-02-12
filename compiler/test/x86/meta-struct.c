/*
 * struct-[pointer]-within-a-struct
 * is WIP in the compiler.
 *
 * this is some test code for this
 *
 * Wed Feb 12 12:22:25 EST 2014
 */

/* XXX:
	not sure if something like
	struct foo {
		struct bar {
			int x;
		};
	} thing;

	should automatically create the foo.bar labels
	i.e. in this case foo.bar, foo.bar.x
	without an explicit variable name for the inner
	struct field. should check my k&r book.

	also, inner struct type references like in

	struct foo {
		struct bar {

		} bar_thing;

		struct bar_thing* pointer;
	}
	
	are apparently currently broken; probably just
	a missing symbol table call somewhere
 */

typedef struct token {
	char type;
	char* start;
	int len;

	/* for elegant parser diagnostics */
	int from_line;	/* originating source line */
	int from_char;	/* originating offset in that line */
} token_t;

typedef struct exp_tree {
	char head_type;
	/* XXX: token_t* tok; */
	struct token* tok;
	int child_count;
	int child_alloc;
	struct exp_tree **child;
} exp_tree_t;

typedef struct stuff {
	struct bagel {
		char* seed_type;
		struct bob {
			int x;
		} bob;
	} bagel;

/*
	struct spaghetti {
		struct bagel* b;
	} spag;
*/
} stuff_t;

main()
{
	stuff_t thing;
	exp_tree_t bob;
	bob.tok = malloc(sizeof(token_t));
	bob.tok->type = 123;

	printf("%d\n", bob.tok->type);


	thing.bagel.seed_type = "sesame";
	thing.bagel.bob.x = 456;
/*
	thing.spag.b = &(thing.bagel);
*/

	puts(thing.bagel.seed_type);
	printf("%d\n", thing.bagel.bob.x);
/*
	puts(thing.spag->b.seed_type);
*/

}
