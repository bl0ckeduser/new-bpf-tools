/*
 * Example of a syntax error being handled
 * by the wannabe c compiler
 */

#include <stdlib.h>
#include <stdio.h>

/* typos here */

typedef struct
{
	/* error: unknown structure when looking for tag `next' */
	struct struct_block *next;
	struct struct_block *prev;
} struct_block;

main()
{
	struct_block_t *cn, *nn;
	cn = malloc(sizeof(struct_block_t));
	cn->next = malloc(sizeof(struct_block_t));

	cn->next->prev = nn;
}
