/*
 * Towers of Hanoi
 * 
 * Practice for an interview
 *
 * October 8, 2014
 */

#include <stdio.h>

typedef struct {
	char stk[32];
	int len;
} stack;

stack stacks[3];

void fail(char *s)
{
	puts(s);
	exit(1);
}

char stack_pop(stack *stk)
{
	if (stk->len == 0)
		fail("tried to pop an empty stack");
	return stk->stk[--stk->len];
}

void stack_push(stack *stk, char i)
{
	if (stk->len && stk->stk[stk->len - 1] > i)
		fail("hanoi ordering condition");
	stk->stk[stk->len++] = i;
}

void dump(stack *ss)
{
	int i, j;
	printf("---------------------------\n");
	for (i = 0; i < 3; ++i) {
		printf("stack %d: ", i);
		for (j = 0; j < ss[i].len; ++j) {
			putchar(ss[i].stk[j]);
		}
		putchar('\n');
	}
}

void setup(int n)
{
	stacks[0].len = stacks[1].len = stacks[2].len = 0;
	int i;

	for (i = 0; i < n; ++i)
		stack_push(&stacks[0], 'a' + i);	
}

void hanoi(int src, int count, int dest)
{
	if (count == 1) {
		char item = stack_pop(&stacks[src]);
		stack_push(&stacks[dest], item);
		dump(stacks);
	} else {
		hanoi(src, count - 1, alt(src, dest));
		hanoi(src, 1, dest);
		hanoi(alt(src, dest), count - 1, dest);
	}
}

int alt(int a, int b)
{
	int i;
	for (i = 0; i < 3; ++i) {
		if (a != i && b != i) {
			return i;
		}
	}
}

main()
{
	int n = 5;
	setup(n);
	dump(stacks);
	hanoi(0, n, 2);
}