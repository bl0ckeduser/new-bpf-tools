/*
 * Test comma-separated sequences
 */

int i, j;

for (i = 2, j = 5; i < j; ++i)
	printf("%d ", i);
printf("\n");

puts(("ha!", "ha?", "check 'em", "hax!"));

i = 2, 3, 4, 5;
printf("%d\n", i);

