/*
 * test for enums
 * Thu Feb 13 20:55:57 EST 2014
 */

enum bob {
	FOO,
	BAR,
	FIVE = 5,
	SIX,
	SEVEN,
	EIGHT
} derp = EIGHT;

void go (int x)
{
	switch (x) {
		case FIVE: 	puts("five"); break;
		case SIX: 	puts("six"); break;
		case SEVEN: 	puts("seven"); break;
		case EIGHT: 	puts("eight"); break;
	}
}

main()
{
	enum bob thing = SEVEN;
	printf("%d %d ", derp, thing);
	while (--thing >= 5)
		printf("%d ", thing);
	printf("\n");
	go(EIGHT);
	go(SEVEN);
	go(SIX);
	go(FIVE);
}
