/*
 * http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf
 * 6.5.17 comma operator
 */

f(a1, a2, a3)
{
	printf("%d, %d, %d\n",
		a1, a2, a3);
}

main()
{
	int a = 1, b = 2, c = 3;
	int t;
	/*
	 * second argument should be 5
	 */
	f(a, (t=3, t+2), c);
}
