int derp[100];
int i;

for (i = 0; i < 10; ++i)
	derp[i] = (i * i) % 2;

for (i = 0; i < 10; ++i)
	if (!derp[i] && i > 5)
		printf("ABC %d\n", i);
