int t = 5;
int a = 1;
int b = 0;

while (t-- > 0) {
	echo(a += b);
	echo(b += a);
}
