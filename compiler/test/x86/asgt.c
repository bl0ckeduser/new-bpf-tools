main() {
int bob[100];

bob[12] = 0;

echo(bob[34] = bob[56] = bob[78] = 123);

echo(bob[12]);
echo(bob[34]);
echo(bob[56]);

echo(bob[12] += bob[34] += bob[56]);

echo(bob[12]);
echo(bob[34]);
echo(bob[56]);
}
