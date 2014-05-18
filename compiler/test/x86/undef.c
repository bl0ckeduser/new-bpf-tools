main()
{
	/*
	 * These #defines are named after the
	 * McKenzie brothers, a pair canadian
	 * television characters who existed
	 * a number of years ago and who devoted
	 * much of their energies towards drinking
	 * beer.
	 *
	 * http://en.wikipedia.org/wiki/Bob_and_Doug_McKenzie
	 */
	#define DOUG "hahaha"
	#define BOB 123
	#undef BOB
	#define BOB 456

	/*
	 * A pretty subtle test case here:
	 * the preprocessor should substitute in the BOB
	 * in the second printf argument correctly here,
	 * because the #undef has not occured yet.
	 * Furthermore, the substitution
	 * should not occur in the string!
	 * So, overall we should get BOB=456.
	 *
	 * As of Sun May 18 14:20:33 EDT 2014 the
	 * preprocessor has just been fixed to
	 * correctly do all this. Previously,
	 * the way it worked was it collected
	 * all defines and then provided them
	 * to the tokenizer after preprocessing
	 * was done. With that method, the #undef
	 * killed the BOB define before the tokenizer
	 * even saw it, and so the substitution never
	 * occured and the final generated executable
	 * went looking for an unexistant "BOB" global
	 * symbol. (When a random symbol is given to the
	 * compiler it defaults to treating it as an undeclared
	 * external symbol; this behaviour
	 * was originally introduced as a hack to get stdin and stderr
	 * without the need for headers)
	 */
	printf("BOB=%d\n", BOB);

	puts(DOUG);

	#undef BOB

	#ifdef BOB
		printf("wrong, wrong, wrong\n");
	#endif
}
