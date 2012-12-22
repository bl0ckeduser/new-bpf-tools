#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"
#include "tokens.h"
#include "tree.h"
#include "parser.h"
#include "codegen.h"

int main(int argc, char** argv)
{
	char* buf;
	token_t* tokens;
	exp_tree_t tree;
	extern void optimize(exp_tree_t *et);
	extern void run_codegen(exp_tree_t* tree);
	extern void push_line(char *lin);
	extern void print_code(void);
	extern void fail(char*);

	if (!(buf = malloc(1024 * 1024)))
		fail("alloc program buffer");

	/* read in at most 1KB of code from stdin */
	fread(buf, sizeof(char), 1024 * 1024, stdin);

	setup_tokenizer();
	tokens = tokenize(buf);

	/* display the tokens */
	/* for (i = 0; tokens[i].start; i++) {
		fprintf(stderr, "%d: %s: ", i, tok_nam[tokens[i].type]);
		tok_display(stderr, tokens[i]);
		fputc('\n', stderr);
	} */

	tree = parse(tokens);
	optimize(&tree);
	
	/* printout_tree(tree);
	 * fputc('\n', stderr);
	 */

	run_codegen(&tree);

	/* write out the final compiled assembly */
	print_code();

	free(buf);
	return 0;
}


