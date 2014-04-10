main() {
int x = 1;
int f;
int m = 1;

while (m++ < 10) {
	f = 1;
	x = 1;
loop:	x *= f++;
	if (f < m)
		goto loop;
	echo(x);
}
}
