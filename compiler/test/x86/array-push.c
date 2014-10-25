/*
 * source: IIRC it was someone asking for help on irc
 *         who sent me this
 * file date: 2012-04-08
 * relevance: a subtle problem:
 *                char bob[] = "ha";
 *  should be coded as
 *                char bob[] = {'h', 'a', '\0'};
 *  as far as i can tell, yet it wasn't
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void list_push(char*** list, char* word)
{
	int siz = 0;
	char** l;
	char** tmp;

	l = *list;
	while(*l){
		l++;
		siz++;
	}
	*l  = malloc(strlen(word) + 1);
	memcpy(*l, word, strlen(word) + 1);

	tmp = realloc(*list, (siz + 2) * sizeof(char *));
	tmp[siz + 1] = NULL;

	*list = tmp;
}

char** make_list(char* words, char* sep)
{
	char* word;
	char** list = malloc(2 * sizeof(char *));
	char** tmp;
	int i = 1;

	word = strtok(words, sep);
	*list = malloc(strlen(word) + 1);
	memcpy(*list, word, strlen(word) + 1);

	while((word = strtok(NULL, sep))) {
		tmp = realloc(list, sizeof(char *) * (++i + 1));
		list = tmp;
		list[i - 1] = malloc(strlen(word) + 1);
		memcpy(list[i - 1],  word, strlen(word) + 1);
	}

	list[i] = NULL;

	return list;
}

int main(int argc, char** argv)
{
	char** list = NULL;
	char** l;
	char words[] = "bob,clown,a";

	/* put stuff in the list */
	list = make_list(words, ",");
	list_push(&list, "haha");
	list_push(&list, "lol");
	list_push(&list, "joke");

	/* print out list */
	l = list;
	while(*l)	puts(*l++);

	/* free list */
	l = list;
	while(*l)	free(*l++);
	free(list);

	return 0;
}
