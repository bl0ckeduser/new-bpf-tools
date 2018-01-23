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
	struct foo haha;
	struct foo w;
	struct foo z;
	struct foo *p = &w;
	struct foo *q = &u;

	u.bar = 123;
	u.baz[3] = 456;
	haha.bar = 999;
	haha.baz[0] = 999;

	v = u;
	*p = *q;
	/* *p = u; */
	z = *p;

	printf("%d%d\n", v.bar, v.baz[3]);
	printf("%d%d\n", u.bar, u.baz[3]);
	printf("%d%d\n", w.bar, w.baz[3]);
	printf("%d%d\n", p->bar, p->baz[3]);
	printf("%d%d\n", z.bar, z.baz[3]);
	printf("%d%d\n", haha.bar, haha.baz[0]);
}
