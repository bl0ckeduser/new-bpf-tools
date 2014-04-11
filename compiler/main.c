/*
 * New compiler targeting my old "BPF" VM
 * and now also 32-bit x86. It could easily
 * target other things given sufficient time.
 *
 * by Bl0ckeduser, December 2012 - February 2014
 * Enjoy !
 */

/*
 * What this file does:
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
#include "general.h"
#include "typedesc.h"
#include "optimize.h"
#include "preprocess.h"
#include <unistd.h>	/* XXX: for dup2; i guess windows might choke on it */

#ifdef WCC
	#include <errno.h>	/* that's business with .NET */
	extern char *tempnam(const char *dir, const char *pfx);
#endif

char* buf, *nbuf;

#ifdef WCC
	char *inf = NULL;
	char opt[1024];
	char *tempf;
	char cmd[1024];
#endif

/*
 * Factored-out core tasks: read from stdin into a buffer;
 * preprocess the buffer, gathering defines; inject
 * compatibility hack defines (NULL=0) to the defines table;
 * tokenize the buffer performing the define-substitutions;
 * finally, parse the tokens.
 */
exp_tree_t run_core_tasks()
{
	token_t* tokens;
	int i, c;
	int alloc = 1024;
	hashtab_t* cpp_defines;

	/* 
	 * Default mode: read code from stdin
	 * into a variable-sized buffer
	 */
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

	/*
	 * Run preprocessor, and collect
	 * defines in `cpp_defines' hash table.
	 */
	cpp_defines = preprocess(&buf);

	/*
	 * (!!!) Compatibility hacks for
	 * some commonly-used things
	 */
	hashtab_insert(cpp_defines, "NULL", strdup("(void *)0"));
	hashtab_insert(cpp_defines, "FILE", strdup("void"));

	/*
	 * Tokenize the inputted code
	 */
	setup_tokenizer();
	tokens = tokenize(buf, cpp_defines);

	#ifdef DEBUG
		/*
		 * Debug mode: display the tokens
		 */
		for (int i = 0; tokens[i].start; i++) {
			fprintf(stderr, "%d: %s: ", i, tok_nam[tokens[i].type]);
			tok_display(tokens[i]);
			fputc('\n', stderr);
		}
	#endif

	/*
	 * Run the parser
	 */
	return parse(tokens);
}

/*
 * Implements AST-dump operation mode:
 * read-preprocess-tokenize-parse, then
 * just dump the parse tree.
 */
void dump_ast()
{
	exp_tree_t tree = run_core_tasks();
	dup2(1, 2);	/* 2>&1 */
	printout_tree(tree);
	fputc('\n', stderr);
	exit(0);
}

/*
 *
 *       This routine performs one iteration of:
 *
 *                    .--------.
 * C code in stdin -> |COMPILER| -> x86 in stdout
 *                    .--------.
 *
 */
void compile_one_file()
{
	extern void run_codegen(exp_tree_t* tree);
	extern void print_code(void);

	exp_tree_t tree = run_core_tasks();

	#ifdef DEBUG
		/*
		 * Debug mode: display the parse-tree
		 */
		printout_tree(tree);
		fputc('\n', stderr);
	#endif

	/*
	 * Optimize the parse-tree
	 */
	optimize(&tree);
	
	#ifdef DEBUG
		/*
		 * Debug mode: display the (optimized) parse-tree
		 */
		printout_tree(tree);
		fputc('\n', stderr);
	#endif

	/* 
	 * Finally, run the codegen and write out the 
	 * final compiled assembly
	 */
	run_codegen(&tree);
	print_code();

	free(buf);
}

int main(int argc, char** argv)
{
	int i;
	#ifdef WCC
		char wcc_sfiles[4096];
	#endif

	/*
	 * wcc (wannabe c compiler) command
	 * for unixlikes running x86
 	 */
	#ifdef WCC
		sprintf(opt, "");
		for (i = 1; i < argc; ++i) {
			if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "--version")) {
				printf("This is the wannabe C compiler command\n");
				printf("programmed by bl0ckeduser, 2014-2014\n");
				printf("<https://github.com/bl0ckeduser/new-bpf-tools>\n\n");
				printf("Usage: wcc filename.c... [-o target]\n\n");
				exit(0);
			} else if (!strcmp(argv[i], "--ast")) {
				dump_ast();
			/* source file(s) */
			} else if(strstr(argv[i], ".c")) {
				inf = argv[i];
				tempf = tempnam("/tmp", "wcc");
				strcat(tempf, ".s");
				freopen(tempf, "w", stdout);
				freopen(inf, "r", stdin);
				compile_one_file();
				fflush(stdout);
				strcat(wcc_sfiles, tempf);
				strcat(wcc_sfiles, " ");
				printf("%s\n", wcc_sfiles);
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
		/*
	 	 * not compiled in WCC -- then
		 * just do the stdin ---> stdout process,
		 * with the option of going in AST-dump mode
		 * through a flag.
		 */

		/* CLI option: dump ast to stdout */
		if (argc > 1 && !strcmp(argv[1], "--ast"))
			dump_ast();

		compile_one_file();
	#endif

	/*
	 * `wcc' (wannabe C compiler) command
	 * for unixlikes running x86: the outputs
	 * are redirected to a temporary .s files,
	 * then assembled using gcc.
	 * (then remove the temporary .s files)
 	 */
	#ifdef WCC
		sprintf(cmd, "gcc -m32 %s -lc %s", wcc_sfiles, opt);
		system(cmd);
		sprintf(cmd, "rm -f %s", wcc_sfiles);
		system(cmd);
	#endif

	return 0;
}


