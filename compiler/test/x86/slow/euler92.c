/* November 28, 2010 */

#include <stdio.h>
#include <stdlib.h>

#define MAX 10000000

int sum_digit_squares(int n)
{
	int s = 0;
	while(n)
	{
		s+= (n%10)*(n%10);
		n/=10;
	}
	return s;
}

int chain(int n)
{
	while(n!=1 && n!=89)
		n = sum_digit_squares(n);
	return n;
}

int main(int argc, char* argv[])
{
	int n = 0;
	int count = 0;
	while(n++<MAX)
		if(chain(n)==89)
			count++;
	printf("%d\n%d\n", MAX, count);
	return 0;
}

