main() {
int bob[10];

bob[5] = 7;
bob[3] = 12;
++bob[5];
echo(++bob[5]);
echo(bob[5]++);
while (++bob[5] < 32) {
	echo(bob[5]++);
	echo(--bob[3]);
	bob[5] += 12;
}

}
