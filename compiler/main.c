/*
 * New compiler targeting my old "BPF" VM
 * and now also 32-bit x86. It could easily
 * target other things given sufficient time.
 *
 * by Bl0ckeduser, December 2012 - ......
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

char* buf, *nbuf;

#ifdef WCC
	char *inf = NULL;
	char opt[1024];
	char *tempf;
	char cmd[1024];
#endif

/* 
 * Used for e.g. wcc foo.c -DFLAG -DTHING=123
 */
struct cli_define_s {
	char *key;
	char *val;
} *cli_defines;
int cli_defines_count = 0;

int shutup_warnings = 0;

char *current_file;

/*
 * Factored-out core tasks: read from stdin into a buffer;
 * preprocess the buffer, gathering defines; inject
 * compatibility hack defines (NULL=0) to the defines table;
 * tokenize the buffer performing the define-substitutions;
 * finally, parse the tokens.
 */
exp_tree_t run_core_tasks(void)
{
	token_t* tokens;
	int i, c;
	int alloc = 1024;
	hashtab_t* cpp_defines = new_hashtab();

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

	#ifdef WCC
		/*
		 * CLI-injected defines
		 */
		for (i = 0; i < cli_defines_count; ++i) {
			hashtab_insert(cpp_defines, cli_defines[i].key, cli_defines[i].val);
		}
	#endif

	/*
	 * Preserve knowledge of beeing on FreeBSD through
	 * self-compilations, because FreeBSD needs special
	 * attention since doesn't work the same as Linux.
	 * it doesn't refer to "stdin" by the symbol-name
	 * "stdin", etc.
	 */
	#ifdef __FreeBSD__
		hashtab_insert(cpp_defines, "__FreeBSD__", my_strdup("1"));
	#endif

	/*
	 * (!!!) Compatibility hacks for
	 * some commonly-used things
	 */
	hashtab_insert(cpp_defines, "NULL", my_strdup("0"));
	hashtab_insert(cpp_defines, "FILE", my_strdup("void"));
	hashtab_insert(cpp_defines, "EOF", my_strdup("-1"));

	/*
	 * Run preprocessor, and collect
	 * defines in `cpp_defines' hash table.
	 */
	preprocess(&buf, cpp_defines);

	/*
	 * Tokenize the inputted code
	 */
	setup_tokenizer();
	tokens = tokenize(buf);

	#ifdef DEBUG
		/*
		 * Debug mode: display the tokens
		 */
		for (i = 0; tokens[i].start; i++) {
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
void dump_ast(void)
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
void compile_one_file(void)
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
		char wcc_buf[256];
		char *p, *q;
		int len;
	#endif

	current_file = "<stdin>";

	init_tokens();
	init_tree();

	/*
	 * wcc (wannabe c compiler) command
	 * for unixlikes running x86
 	 */
	#ifdef WCC
		cli_defines = malloc(100 * sizeof(struct cli_define_s));
		if (!cli_defines)
			fail("malloc");
		sprintf(opt, "");
		for (i = 1; i < argc; ++i) {
			if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "--version")) {
				printf("This is the wannabe C compiler command, version XXX\n");
				printf("programmed by bl0ckeduser, 2012-2014\n");
				printf("<https://github.com/bl0ckeduser/new-bpf-tools>\n\n");
				printf("Usage: wcc filename.c... [-o target] [-Ddef[=val]]... [-w]\n\n");
				exit(0);
			} else if (!strcmp(argv[i], "--ast")) {
				dump_ast();
			} else if (!strcmp(argv[i], "-w")) {
				shutup_warnings = 1;
			/* source file(s) */
			} else if(strstr(argv[i], ".c")) {
				current_file = inf = argv[i];
				tempf = tempnam("/tmp", "wcc");
				strcat(tempf, ".s");
				if (!freopen(tempf, "w", stdout))
					fail("stdout redirection");
				if (!freopen(inf, "r", stdin))
					fail("stdin redirection");
				compile_one_file();
				fflush(stdout);
				strcat(wcc_sfiles, tempf);
				strcat(wcc_sfiles, " ");
				printf("%s\n", wcc_sfiles);
			/* #define injection */
			} else if (argv[i][0] && argv[i][0] == '-' && argv[i][1] == 'D') {
				p = &argv[i][2];
				for(len = 0, q = wcc_buf; *p && *p != '=' && len < 255; ++p, ++q, ++len)
					*q = *p;
				*q = 0;
				cli_defines[cli_defines_count].key = my_strdup(wcc_buf);
				if (*p) {
					++p;	/* eat '=' */
					for(len = 0, q = wcc_buf; *p && len < 255; ++p, ++q, ++len)
						*q = *p;
					*q = 0;
					cli_defines[cli_defines_count].val = my_strdup(wcc_buf);
				} else {
					cli_defines[cli_defines_count].val = my_strdup(" ");
				}
				++cli_defines_count;
			/* other options, get passed down to gcc assembler/linker */
			} else {
				strcat(opt, argv[i]);
				strcat(opt, " ");
			}
		}
		if (!inf)
			fail("expected a .c file as an argument");
		if (!freopen(inf, "r", stdin)) {
			sprintf(cmd, "could not open input file `%s'.", inf);
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
		if (system(cmd) == -1)
			fail("Failed to assemble. Do you have gcc installed ?");
		sprintf(cmd, "rm -f %s", wcc_sfiles);
		if (system(cmd) == -1) {
			fprintf(stderr, "Couldn't remove temporary files...\n");
		}
	#endif

	return 0;
}


