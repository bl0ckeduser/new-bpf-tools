main() {
int i;
int j;

i = 1;
while (i <= 4) {
	j = 1;
	while (j <= 4)
		echo(i * j++);
	++i;
}
}
