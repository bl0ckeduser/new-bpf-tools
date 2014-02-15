/*
 * Hash table (keys are strings)
 */

#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

extern void fail(char *);

/*
 * Hash function. mostly just random nonsense for now,
 * I am no expert on hash functions.
 */
unsigned int hashtab_hash(char *key, int nbuck)
{
	unsigned int hash = 0;
	unsigned int prev_key = *key;
	while (*key) {
		hash += (*key - prev_key) * 100;
		hash += (prev_key = *key++);
		hash %= nbuck;
	}
	return hash;
}

hashtab_t* new_hashtab(void)
{
	hashtab_t* htab;
	int i;

	htab = malloc(sizeof(hashtab_t));
	if (!htab)
		fail("couldn't allocate a hash table");

	htab->nbuck = 241;	/* prime */
	htab->buck = malloc(htab->nbuck * sizeof(hashtab_entry_t *));
	if (!(htab->buck))
		fail("couldn't allocate hash table buckets");
	for (i = 0; i < htab->nbuck; ++i)
		htab->buck[i] = NULL;

	return htab;
}

void* hashtab_lookup(hashtab_t* htab, char* key)
{
	unsigned int hash = hashtab_hash(key, htab->nbuck);
	hashtab_entry_t *ptr = htab->buck[hash];

	while (ptr)
		if (!strcmp(ptr->key, key))
			break;
		else
			ptr = ptr->next;

	if (ptr && strcmp(ptr->key, key))
		return NULL;
	else if (ptr)
		return ptr->val;
	else
		return NULL;
}

void hashtab_insert(hashtab_t* htab, char* key, void* val)
{
	unsigned int hash = hashtab_hash(key, htab->nbuck);
	hashtab_entry_t **ptr = &(htab->buck[hash]);

	while (*ptr)
		ptr = &((*ptr)->next);

	*ptr = malloc(sizeof(hashtab_entry_t));
	if (!*ptr)
		fail("couldn't create a hash table bucket node");
	(*ptr)->key = malloc(strlen(key) + 1);
	if (!((*ptr)->key))
		fail("couldn't allocate a string");
	strcpy((*ptr)->key, key);
	(*ptr)->val = val;
}
