main()
{
	#define DOUG "hahaha"
	#define BOB 123
	#undef BOB
	#define BOB 456

/*
 * XXX: this should work --
 * the preprocessor should substitue in the BOB
 * before the undef occurs, which currently it doesn't
 */
	printf("BOB=%d\n", BOB);
	
	puts(DOUG);

	#undef BOB

	#ifdef BOB
		printf("wrong, wrong, wrong\n");
	#endif
}
