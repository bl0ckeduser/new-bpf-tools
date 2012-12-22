int bob[10];

bob[5] = 7;
++bob[5];
echo(++bob[5]);
echo(bob[5]++);
while (++bob[5] < 32) {
	echo(bob[5]++);
	bob[5] += 12;
}

