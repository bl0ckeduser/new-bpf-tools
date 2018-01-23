#define FOO 3
#define BAR 1 + 2 * FOO

main()
{

	int x = FOO;
	int y = BAR;

	printf("%d, %d, %d\n", x, y, FOO + BAR);
}
