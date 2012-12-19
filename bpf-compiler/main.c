#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"
#include "tokens.h"
#include "tree.h"
#include "parser.h"

int main(int argc, char** argv)
{
	char buf[1024 * 1024];
	char buf2[1024];
	token_t* tokens;
	int i;
	exp_tree_t tree;
	extern void optimize(exp_tree_t *et);

	/* read in at most 1KB of code from stdin */
	fread(buf, sizeof(char), 1024 * 1024, stdin);

	setup_tokenizer();
	tokens = tokenize(buf);

	/* display the tokens */
	for (i = 0; tokens[i].start; i++) {
		printf("%d: %s: ", i, tok_nam[tokens[i].type]);
		tok_display(tokens[i]);
		putchar('\n');
	}

	tree = parse(tokens);
	printout_tree(tree);
	putchar('\n');
	optimize(&tree);
	printout_tree(tree);
	putchar('\n');

	codegen(&tree);

	return 0;
}


