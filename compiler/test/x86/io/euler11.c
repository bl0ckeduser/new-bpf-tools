/*
 * Solution to Project Euler problem #11,
 * "Largest product in a grid".
 *
 * The code is pretty dumb except it has
 * fairly big expressions in it which is
 * good for checking how good the compiler
 * is at not overflowing its expression
 * stack !
 *
 * The input grid, exactly as it appears at
 * http://projecteuler.net/problem=11,
 * must be piped into this program.
 *
 * Original C version written October 1st, 2010.
 * Changes marked with XXX
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	/* Input */
	/*
 	 * XXX: the original code had "int data[400]"
	 * below, a bug which happened not to cause
	 * any symptoms when compiling with gcc/amd64,
	 * gcc/x86, clang/amd64, and gcc/ppc32,
	 * but which leads to an incorrect answer when
	 * compiling with my wannabe-compiler.
	 *
	 * I'm feeling increasingly apologetic for
	 * java with all its bounds-checking, and
	 * increasingly scared of relying on
	 * the behaviours of specific C compilers.
	 * I guess it's important to test with several
	 * compilers and perhaps run tools like valgrind
	 * or whatever...
	 */
	int data[401];
	int i = 1;
	int x, y;
	int max = 0;

	while(i<=400)
	{
		scanf("%d", &data[i]);
		i++;
	}

	/* +/+ diagonals */
	for(y = 1; y<=20-3; y++)
		for(x = 1; x<=20-3; x++)
			if( data[cell(x,y)] * data[cell(x+1, y+1)] * data[cell(x+2, y+2)] * data[cell(x+3, y+3)] > max)
				max = data[cell(x,y)] * data[cell(x+1, y+1)] * data[cell(x+2, y+2)] * data[cell(x+3, y+3)];
	/* +/- diagonals */
	for(y = 20; y>=4; y--)
		for(x = 1; x<=20-3; x++)
			if( data[cell(x,y)] * data[cell(x+1, y-1)] * data[cell(x+2, y-2)] * data[cell(x+3, y-3)] > max)
				max = data[cell(x,y)] * data[cell(x+1, y-1)] * data[cell(x+2, y-2)] * data[cell(x+3, y-3)];
	/* -/- diagonals */
	for(y = 20; y>=4; y--)
		for(x = 20; x>=4; x--)
			if( data[cell(x,y)] * data[cell(x-1, y-1)] * data[cell(x-2, y-2)] * data[cell(x-3, y-3)] > max)
				max = data[cell(x,y)] * data[cell(x-1, y-1)] * data[cell(x-2, y-2)] * data[cell(x-3, y-3)];
	/* -/+ diagonals */
	for(y = 1; y<=20-3; y++)
		for(x = 20; x>=4; x--)
			if( data[cell(x,y)] * data[cell(x-1, y+1)] * data[cell(x-2, y+2)] * data[cell(x-3, y+3)] > max)
				max = data[cell(x,y)] * data[cell(x-1, y+1)] * data[cell(x-2, y+2)] * data[cell(x-3, y+3)];
	/* Horizontal + lines */
	for(y = 1; y<=20; y++)
		for(x = 1; x<=20-3; x++)
			if( data[cell(x,y)] *  data[cell(x+1,y)] * data[cell(x+2,y)] * data[cell(x+3,y)] > max)
				max = data[cell(x,y)] *  data[cell(x+1,y)] * data[cell(x+2,y)] * data[cell(x+3,y)];
	/* Horizontal - lines */
	for(y = 1; y<=20; y++)
		for(x = 20; x>=4; x--)
			if( data[cell(x,y)] *  data[cell(x-1,y)] * data[cell(x-2,y)] * data[cell(x-3,y)] > max)
				max = data[cell(x,y)] *  data[cell(x-1,y)] * data[cell(x-2,y)] * data[cell(x-3,y)];
	/* Vertical + lines */
	for(y = 1; y<=20-3; y++)
		for(x = 1; x<=20; x++)
			if( data[cell(x,y)] *  data[cell(x,y+1)] * data[cell(x,y+2)] * data[cell(x,y+3)] > max)
				max = data[cell(x,y)] *  data[cell(x,y+1)] * data[cell(x,y+2)] * data[cell(x,y+3)];
	/* Vertical - lines */
	for(y = 20; y>=4; y--)
		for(x = 1; x<=20; x++)
			if( data[cell(x,y)] *  data[cell(x,y-1)] * data[cell(x,y-2)] * data[cell(x,y-3)] > max)
				max = data[cell(x,y)] *  data[cell(x,y-1)] * data[cell(x,y-2)] * data[cell(x,y-3)];

	printf("%d\n", max);
 
	return 0;
}

int cell(int x, int y)
{
	if(x<=20 && x>=0 && y<=20 && y>=0)
	{
		return 20*(y-1)+x;
	} else {
		printf("BAD CELL ACCESS ! %d, %d\n", x, y);
		return 0;
	}
}
