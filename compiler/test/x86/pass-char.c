/* test mixed-size arguments */

func(char a, char b, char c)
{
	printf("%c%c%c\n", a, b, c);
}

mixed(int i0, char c1, int i1, char c2, int i2)
{
	printf("%c %c ", c1, c2);
	printf("%d %d %d\n", i0, i1, i2);
}

main() {
	char i, j, k;

	func('a', 'b', 'c');

	i = 'A';
	j = 'B';
	k = 'C';

	func(i, j, k);

	mixed(9876543, i, 123456, j, 7891234);
}
