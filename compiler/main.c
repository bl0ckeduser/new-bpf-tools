/*
 * New compiler targeting my old "BPF" VM
 * and now also 32-bit x86. It could easily
 * target other things given sufficient time.
 *
 * by Bl0ckeduser, December 2012 - August 2013
 * Enjoy !
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tokenizer.h"
#include "tokens.h"
#include "tree.h"
#include "parser.h"
#include "typedesc.h"

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

	/* Read in at most 1KB of code from stdin */
	if (!(buf = malloc(1024 * 1024)))
		fail("alloc program buffer");
#ifdef SELF_COMPILATION_HACK
	int i, c;
	for (i = 0; i < 1024 * 1024; ++i) {
		c = getchar();
		if (c < 0) {
			buf[i] = 0;
			break;
		}
		buf[i] = c;
	}
#else
	fread(buf, sizeof(char), 1024 * 1024, stdin);
#endif

	setup_tokenizer();
	tokens = tokenize(buf);

#ifdef DEBUG
	/* display the tokens */
	for (int i = 0; tokens[i].start; i++) {
		fprintf(stderr, "%d: %s: ", i, tok_nam[tokens[i].type]);
		tok_display(tokens[i]);
		fputc('\n', stderr);
	}
#endif

	tree = parse(tokens);

#ifdef DEBUG
	printout_tree(tree);
	fputc('\n', stderr);
#endif

	optimize(&tree);
	
#ifdef DEBUG
	printout_tree(tree);
	fputc('\n', stderr);
#endif

	run_codegen(&tree);

	/* Write out the final compiled assembly */
	print_code();

	free(buf);
	return 0;
}


