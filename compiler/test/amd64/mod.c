/* test % operator */

main()
{
	int a = 123;
	int b = 456;

	printf("%d\n", a % b);
	printf("%d\n", b % a);
	printf("%d\n", b % a % a);
	printf("%d\n", b % a % b);
}
