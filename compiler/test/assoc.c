// Arithmetic is left-associative !
echo(10 / 5/ 2);
echo(20 / 2 / 2 / 2 / 2);
echo(5 - 1 - 1 - 1 - 1 - 1);
echo(3 - 2 - 1);

// Let's still check that these compile right
echo(10/(5/2));
echo(20 / 2 / (2 / 2) / 2);
echo(5 - 1 - 1 - (1 - 1 - 1));
echo(3 - (2 - 1));

