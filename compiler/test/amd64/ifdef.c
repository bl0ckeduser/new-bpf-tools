/*
 * Test for WIP preprocessor
 * Nested #ifdef / #ifndef
 * Thu Apr 10 21:09:09 EDT 2014
 */

#define BOB
/* #define DONALD */

main()
{

#ifndef BOB
	printf("BOB is undefined");
#else
	#ifdef BOB
		printf("BOB is defined\n");
		#ifdef DONALD
			printf("DONALD is defined\n");
		#else
			printf("DONALD is undefined\n");
			#ifdef BOB
				printf("BOB is defined\n");
				#ifndef DONALD
					printf("DONALD is undefined\n");
				#else
					printf("DONALD is defined\n");
				#endif
			#else
				printf("BOB is undefined\n");
			#endif
		#endif
	#else
		printf("BOB is not defined\n");
	#endif
#endif

}
