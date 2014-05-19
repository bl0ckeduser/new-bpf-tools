/*
 * parameterized defines
 */

#define bob(x,y) x+y

main()
{
	/* recursive substiution support is important */
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
