/*
 * Project Euler, Problem 21
 *
 * This is based on the solution I wrote in C at some point
 * in 2010, at which point I was a greater noob than now.
 */

sumDivisors(i)
{
	int x = 0;
	int n = i;
	while(n-->1)
		if((i%n)==0)
			x+=n;
	return x;
}

main(argc, argv)
{
	int *done = malloc(10000 * 4);
	int a = 1;
	int b;
	int theSum = 0;
	int c = 10000;
	while(c--)
		done[c]=0;

	while(a<10000)
	{
		b = sumDivisors(a);
			if(a!=b && !done[a] && sumDivisors(b)==a)
			{
				printf(">>> %d %d\n", a, b);
				done[b]=1;
				theSum+=(a+b);
			}
		a++;
	}

	printf("Result : %d\n", theSum);

	return 0;
}
