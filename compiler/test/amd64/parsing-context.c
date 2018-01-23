/*
 * The meaning of [] depends upon context.
 * There are two cases:
 * - e.g. int foo[] = {1, 2, 3} -- it means an array
 *   the size of which is to be figuerd out by the
 *   compiler based on the initializer
 *
 * - e.g. char ptr[] = "Enjoy your teeth" -- it's
 *   an ancient notation for a pointer star '*'
 *
 * The `contains_ambig' variable is set in this case,
 * which delays the adding of any tree node to
 * a later point in time, a point which occurs
  * a few tokens ahead and at which the context
 * of [] has been inferred.
 */

main()
{
	char *happy2014 = "hohoho ! happy new year 2014 to you";
	char* name[] = {"Roaldn", "McOdandls"};
	char old_school_ptr[] = "hahaha !!!!";

	puts(happy2014);
	printf("%s %s\n", name[0], name[1]);
	puts(old_school_ptr);
}
