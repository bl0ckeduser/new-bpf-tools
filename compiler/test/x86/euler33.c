/*
 * Project euler problem 33
 * Code written October 30, 2010
 */

/* ------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>

void main(void)
{
	int test;
	int x, keycount, y, num_digit1;
	int num_digit2, denom_digit1, denom_digit2;
	int test2, c;
	int *num_keys = malloc(100 * sizeof(int));
	int *denom_keys = malloc(100 * sizeof(int));
	int *factor_count = malloc(1000 * sizeof(int));
	int product;
	int denom;
	int tp;
	
	int *numFactors = malloc(10000 * sizeof(int));
	int *denomFactors = malloc(10000 * sizeof(int));

	x = 10;

	keycount = 0;
	
	while(x<100)
	{
		y = 10;
		while(y<x)
		{
			if((x%10!=0)*(y%10!=0))
			{
				num_digit1 = (y/10);
				num_digit2 = y-(10*(y/10));
				
				denom_digit1 = (x/10);
				denom_digit2 = x-(10*(x/10));
			
				if(num_digit1==denom_digit1)
				{
					test = y*1000/x;
					test2 = num_digit2*1000/denom_digit2;
					if(test==test2)
					{
						keycount++;
						num_keys[keycount] = y;
						denom_keys[keycount] = x;
					}
				}	
	
				if(num_digit1==denom_digit2)
				{
					test = y*1000/x;
					test2 = num_digit2*1000/denom_digit1;
					if(test==test2)
					{
						keycount++;
						num_keys[keycount] = y;
						denom_keys[keycount] = x;
					}
				}
		
				if(num_digit2==denom_digit1)
				{
					test = y*1000/x;
					test2 = num_digit1*1000/denom_digit2;
					if(test==test2)
					{
						keycount++;
						num_keys[keycount] = y;
						denom_keys[keycount] = x;
					}
				}
	
				if(num_digit2==denom_digit2)
				{
					test = y*1000/x;
					test2 = num_digit1*1000/denom_digit1;
					if(test==test2)
					{
						keycount++;
						num_keys[keycount] = y;
						denom_keys[keycount] = x;
					}
				}
			}
	
			y++;
		}
		x++;
	}
	
	c = 0;
	denom = 1;
	product = 1;
	while(c++<keycount)
	{
		printf("%d / %d\n", num_keys[c], denom_keys[c]);
		denom*=denom_keys[c];
		product*=num_keys[c];
	}
	
	printf("Product (not simplified): %d / %d \n", product, denom);
	
	tp = 1;
	while(tp++<denom)
	{
		while(denom%tp==0 && product%tp==0)
		{
			product/=tp;	
			denom/=tp;
			printf("Product: %d / %d - %d\n", product, denom, tp);
		}
	}
}

		