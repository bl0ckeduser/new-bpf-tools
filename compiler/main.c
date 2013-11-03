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
#include <unistd.h>	/* XXX: for dup2; i guess windows might choke on it */

int main(int argc, char** argv)
{
	char* buf, *nbuf;
	token_t* tokens;
	exp_tree_t tree;
	extern void optimize(exp_tree_t *et);
	extern void run_codegen(exp_tree_t* tree);
	extern void push_line(char *lin);
	extern void print_code(void);
	extern void fail(char*);
	int i, c;
	int alloc = 1024;
	int dump_ast = 0;

	/* CLI option: dump ast to stdout */
	if (argc > 1 && !strcmp(argv[1], "--ast"))
		dump_ast = 1;

	/* Read code from stdin */
	if (!(buf = malloc(alloc)))
		fail("alloc program buffer");
	for (i = 0 ;; ++i) {
		c = getchar();
		if (c < 0) {
			buf[i] = 0;
			break;
		}
		if (i >= alloc) {
			alloc += 1024;
			nbuf = realloc(buf, alloc);
			if (!nbuf)
				fail("realloc failed");
			else
				buf = nbuf;
		}
		buf[i] = c;
	}

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

	/* --ast flag: dump ast to stdout */
	if (dump_ast) {
		dup2(1, 2);	/* 2>&1 */
		printout_tree(tree);
		fputc('\n', stderr);
		exit(0);
	}

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


