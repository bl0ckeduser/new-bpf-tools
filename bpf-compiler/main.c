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
		strncpy(buf2, tokens[i].start, tokens[i].len);
		buf2[tokens[i].len] = 0;
		printf("%s: %s\n", tok_nam[tokens[i].type],
			buf2);
	}

	parse(tokens);
	return 0;
}


