/*
 * Sun Oct 19 14:12:49 EDT 2014
 *
 * yes this loser blockeduser is still alive
 * still wasting time on his wannabe c compiler
 * muahahha
 *
 * anyway: pointer differences are funny beasts.
 * small glitch now fixed.
 */
main()
{
	int bob[] = {1,2,3,4,5,6,7,8,9,10};
	int *a = &bob[3];
	int *b = &bob[7];

	/* it used to give 16 here before the fix */
	printf("%d = 4 ?\n", b - a);
}
