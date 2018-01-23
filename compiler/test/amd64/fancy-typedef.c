/*
 * this code is adapted from some code
 * that was posted to a kind of "group
 * debugging" session on a C programming
 * channel on an IRC server
 *
 * Now it's interesting because it uses
 * typedefs in a way that made my compiler
 * choke prior to a few recent commits
 */

#include <stdio.h>
#include <stdlib.h>

typedef struct resource_list resource_list;
struct resource_list {
	int num;
	resource_list *next;
};

int resource_seen(resource_list **list, int value)
{
	resource_list *ptr;

	/* No list case */
	if (*list == 0x0) {
		printf("  New list\n  Inserting %d\n\n", value);
		*list = malloc(sizeof(resource_list));
		(*list)->num = value;
		(*list)->next = 0x0;
		return 0;
	}

	/* Search */
	for (ptr = *list ; ptr != 0x0; ptr = ptr->next) {
		printf("  Found %d\n", ptr->num);
		if (ptr->num == value) return 1;
		if (ptr->next == 0x0) break;
	}

	/* Not found, insert */
	printf("  No match.  Inserting %d\n\n", value);
	ptr->next = malloc(sizeof(resource_list));
	if (ptr->next == 0x0) {
		printf("malloc failed\n");
		exit(-1);
	}
	ptr->next->num = value;
	ptr->next->next = 0x0;
	return 0;
}

int main (int argc, char **argv)
{
	resource_list *foo = 0x0;

	while (1) {
		if (resource_seen(&foo, 1)) break;
		if (resource_seen(&foo, 2)) break;
		if (resource_seen(&foo, 3)) break;
		if (resource_seen(&foo, 4)) break;
		if (resource_seen(&foo, 2)) break;
		printf("Whoops! Something's wrong here!\n");
		break;
	}

	printf("Done\n");
}

