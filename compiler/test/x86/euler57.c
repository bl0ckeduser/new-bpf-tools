/*
 * ProjectEuler problem 57
 *
 * Original C version written on December 8, 2010
 *
 * Tweaked to compile in wannabe compiler, August 23, 2013
 */

int DIGITS = 2028;
int VERBOSE = 0;

bn_length(char* bn)
{
	int l = 0;
	while(*bn++)
		l++;
	return l;
}

bn_copy(char* bn1, char* bn2)
{
	int i = 0;
	strcpy(bn1, bn2);
	i = bn_length(bn1);
	while(i++<=DIGITS)
		*(bn1+i) = 0;
}

doCarry(char* bn)
{
	int index = 0;
	while(*(bn+index) && index<DIGITS)
	{
		while(*(bn+index)-1>9)
		{
			*(bn+index) -= 10;

			if(!*(bn+index+1))
				*(bn+index+1) = 1;
						
			*(bn+index+1) += 1;
		}
		index++;
	}
}


AddBigNums(char* bn1, char* bn2)
{
	int index = 0;
	while(*(bn2+index) && index<DIGITS)
	{
		if(!*(bn1+index))
			*(bn1+index) = 1;

		*(bn1+index) += *(bn2+index) - 1;
		doCarry(bn1);
		index++;
	}
}

IncBigNum(char* bn)
{
	*(bn)+=1;
	doCarry(bn);
}

CreateBigNum()
{
	return malloc(DIGITS);
}

OutputBigNum(char* bn)
{
	int i = bn_length(bn) - 1;
	while(i+1)
	{
		putchar('0'+*(bn+i)-1);
		i--;
	}
}

DestroyBigNum(char* theBigNum)
{
	free(theBigNum);
}

zerobuf(char *p)
{
	int i;
	for (i = 0; i < DIGITS; ++i)
		p[i] = 0;
}

main(int argc, char** argv)
{
	int null = (char *)0x0;
	char* bigCharA = null;
	char* bigCharB = null;
	char* bigCharC = null;
	char* bigCharD = null;

	int count = 0;
	int term = 1000;

	int i;

	bigCharA = (char *)CreateBigNum();
	bigCharB = (char *)CreateBigNum();
	bigCharC = (char *)CreateBigNum();
	bigCharD = (char *)CreateBigNum();

	for (i = 0; i < DIGITS; ++i) {
		bigCharA[i] = 0;
		bigCharB[i] = 0;
		bigCharC[i] = 0;
		bigCharD[i] = 0;
	}

	if(!bigCharA || !bigCharB || !bigCharC || !bigCharD)
	{
		printf("Could not allocate memory !\n");
		return 0;
	}

	/* ------------------------------------------------- */

	*bigCharA = 2;	/* one */
	*bigCharB = 1;	/* zero */

	while(term--)
	{
		zerobuf(bigCharC);
		*bigCharC = 1;	/* zero */

		/* c = 2*a+b */
		AddBigNums(bigCharC, bigCharA);			
		AddBigNums(bigCharC, bigCharA);			
		AddBigNums(bigCharC, bigCharB);

		/* print c+a, "/" */
		zerobuf(bigCharD);
		*bigCharD = 1;		/* zero */
		AddBigNums(bigCharD, bigCharC);
		AddBigNums(bigCharD, bigCharA);
		if (VERBOSE) {
			OutputBigNum(bigCharD);
			printf(" / ");
		}

		/* print c */
		if (VERBOSE)
			OutputBigNum(bigCharC);

		/* count, newline */
		if(bn_length(bigCharD)>bn_length(bigCharC))
		{
			++count;
			if (VERBOSE)
				printf(" !! ");
		}

		if (VERBOSE)
			printf("\n");

		/* b = a */
		bn_copy(bigCharB, bigCharA);

		/* a = c */
		bn_copy(bigCharA, bigCharC);		
	}

	/* ------------------------------------------------- */

	/*
	 * XXX: the destroy() stuff segfaults on the wannabe
	 * compiler. not sure why. all it does is call free().
	 * anyway a modern unix/windows nt/donald operating system
	 * will automatically get rid of that stuff on exit.
	 */

	/*
	DestroyBigNum(bigCharA);
	DestroyBigNum(bigCharB);
	DestroyBigNum(bigCharC);
	DestroyBigNum(bigCharD);
	*/

	printf("%d\n", count);
	return 0;
}
