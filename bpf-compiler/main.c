#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"
#include "tokens.h"

int main(int argc, char** argv)
{
	char buf[1024];
	char buf2[1024];
	token_t* tokens;
	extern token_t* tokenize(char *string);
	extern void setup_tokenizer();
	extern void parse(token_t *);
	int i;

	/* get a test line string from stdin */
	fgets(buf, 1024, stdin);

	setup_tokenizer();
	tokens = tokenize(buf);

	/* display the tokens */
	for (i = 0; tokens[i].start; i++) {
		printf("%d: %s: ", i, tok_nam[tokens[i].type]);
		tok_display(tokens[i]);
		putchar('\n');
	}

	parse(tokens);
	return 0;
}


