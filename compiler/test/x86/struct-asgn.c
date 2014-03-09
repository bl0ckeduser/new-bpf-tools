/*
 * Struct assignment
 * Sun Mar  9 14:37:14 EDT 2014
 */

main()
{
	struct foo {
	        int bar;
		int baz[9];
	};

	struct foo u;
	struct foo v;
	struct foo w;
	struct foo *p = &w;
	struct foo *q = &u;

	u.bar = 123;
	u.baz[3] = 456;

	v = u;
	/* XXX: *p = *q; */
	/* XXX: *p = u; */

	printf("%d%d\n", v.bar, v.baz[3]);
	printf("%d%d\n", u.bar, u.baz[3]);
	/* XXX: printf("%d%d\n", w.bar, w.baz[3]); */
	/* XXX: printf("%d%d\n", p->bar, p->baz[3]); */
}
