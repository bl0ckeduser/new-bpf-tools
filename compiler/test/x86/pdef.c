/*
 * parameterized defines
 */

#define bob(x,y) x+y
#define doug(a,b) a*b
#define crappy_max(a,b) ((a) > (b) ? (a) : (b))

main()
{
	printf("%d\n", crappy_max(100, 200));
	printf("%d\n", crappy_max(200, 200));
	printf("%d\n", crappy_max(200, 100));
	printf("%d\n", crappy_max(100, 100));

	/* recursive substiution support is important */
	printf("%d\n", doug(bob(123,456), doug(123, 456)));
	printf("%d\n", bob(123,456));
	printf("%d\n", bob( bob(123,456), bob(123,456) ));
	printf("%d\n", bob( bob(bob(123,456),456), bob(123,456) ));
	printf("%d\n", bob( bob(bob(123,bob(123,456)),456), bob(123,456) ));

	/* XXX: this one doesn't work yet */
	/*
	printf("%d\n", bob(	bob(123,456), 
				bob(123,456) ));
	*/
}
