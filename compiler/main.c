/*
 * New compiler targeting my old "BPF" VM
 * and now also 32-bit x86. It could easily
 * target other things given sufficient time.
 *
 * by Bl0ckeduser, December 2012 - February 2014
 * Enjoy !
 */

/*
 * what this file does:
 * - deal with CLI arguments
 * - call the lexer, parser, and codegen in order
 * - a bunch of ugly boring crap like counting memory,
 *   checking for IO errors, etc.  
 * tonight, we go to the far west.
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

#ifdef WCC
	#include <errno.h>	/* that's business with .NET */
#endif

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
#ifdef WCC
	char *inf = NULL;
	char opt[1024];
	char *tempf;
	char cmd[1024];
#endif

	/*
	 * wcc (wannabe c compiler) command
	 * for unixlikes running x86
 	 */
	#ifdef WCC
		sprintf(opt, "");
		for (i = 1; i < argc; ++i) {
			if (!strcmp(argv[i], "--ast")) {
				dump_ast = 1;
			/* unique (for now) source file */
			} else if(strstr(argv[i], ".c")) {
				if (inf) {
					fail("I don't do multiple sourcefiles");
				}
				inf = argv[i];
			}
			/* options, get passed down to gcc assembler/linker */
			else {
				strcat(opt, argv[i]);
				strcat(opt, " ");
			}
		}
		if (!inf)
			fail("expected a .c file as an argument");
		if (!freopen(inf, "r", stdin)) {
			sprintf(cmd, "could not open input file `%s': %s", inf, strerror(errno));
			fail(cmd);
		}
	#else
		/* CLI option: dump ast to stdout */
		if (argc > 1 && !strcmp(argv[1], "--ast"))
			dump_ast = 1;
	#endif

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

	/*
	 * wcc (wannabe c compiler) command
	 * for unixlikes running x86
 	 */
	#ifdef WCC
		tempf = tempnam("/tmp", "wcc");
		strcat(tempf, ".s");
		freopen(tempf, "w", stdout);
	#endif

	run_codegen(&tree);

	/* Write out the final compiled assembly */
	print_code();

	/*
	 * wcc (wannabe c compiler) command
	 * for unixlikes running x86
 	 */
	#ifdef WCC
		fflush(stdout);
		fclose(stdout);
		sprintf(cmd, "gcc -m32 %s -lc %s", tempf, opt);
		system(cmd);
		remove(tempf);
	#endif

	free(buf);

	return 0;
}


