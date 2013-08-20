
int i = 3;

for (;;) {
	echo(i--);
	if (i < 0)
		goto hahaha;
}

hahaha:

for (i = 0; ;++i) {
	echo(i);
	if (i > 5)
		goto hohoho;
}

hohoho:

for (i = 0; i < 5; ++i)
	echo(++i);

