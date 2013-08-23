/*
 * Print all permutations of a string,
 * using a tree-recursive routine. I
 * bet there are smart algorithms out
 * there that beat this. I did not look
 * up an algorithm when writing this.
 *
 * Bl0ckeduser
 * June 26, 2012
 *
 * Tweaked to work with wannabe compiler
 * August 23, 2013
 */

fail(s) {
	puts((char *)s); exit(1);
}

permut(n, i_avail, i_prev)
{
	/* XXX: clean up (char *) mess when compiler adds arg types */
	char *avail = (char *)i_avail;
	char *prev = (char *)i_prev;
	int i, j;
	char child_avail[100];
	char chars[100];

	/* go through all available characters */
	for(i = 0; avail[i]; i++)
		if(avail[i] != '!') {
			/* found an available character;
			   cross it out from the child's
			   available characters list */
			strcpy(child_avail, avail);
			child_avail[i] = '!';

			/* add the character to this
			   permutation-string */
			strcpy(chars, prev);
			strncat(chars, &avail[i], 1);

			/* print out permutation-string
			   if a leaf is reached; otherwise,
			   spawn child */
			if(n == 1)
				puts(chars);
			else
				permut(n - 1, child_avail, chars);
		}
}

main()
{
	char set[100];

	if(scanf("%s", set) < 0)
		fail("failed to input string");

	if(strlen(set) > 100)
		fail("string bigger than hardcoded limit");

	if(strstr(set, "!"))
		fail("! is used internally");

	permut(strlen(set), set, "");
}

