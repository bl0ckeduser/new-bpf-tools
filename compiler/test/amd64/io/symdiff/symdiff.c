/*
 * Modified version of my symbolic
 * differentiator project "symdiff"
 * (https://github.com/bl0ckeduser/symdiff),
 * with some syntaxes tweaked to similar
 * syntaxes that are the only ones of a given kind
 * that currently compile, and also modified to fit in
 * a single file.
 *
 * Sun Mar 16 11:44:13 EDT 2014
 */

/*
 * Note, to port this to amd64 there was one thing that was
 * changed from int to long and one thing from %d to %ld
 * in a sscanf
 */

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

void fail(char* mesg);

char buf_a[128], buf_b[128], buf_c[128];
char *code_lines[1024];
void fail(char* mesg);
void sanity_requires(int exp);
char buf[128];

void fail(char* mesg);

#ifndef GC_H
#define GC_H

typedef struct {
	int alloc;
	int memb;
	void **pile;
} gcarray_t;

//gcarray_t* new_gca(void);
void *cgc_malloc(long size);

#endif


#ifndef TOKENIZER_H
#define TOKENIZER_H

typedef struct token {
	char type;
	char* start;
	int len;

	/* for elegant parser diagnostics */
	int from_line;	/* originating source line */
	int from_char;	/* originating offset in that line */
} token_t;



token_t* tokenize(char *string);
/* void setup_tokenizer(); */

#endif

#ifndef TOKENS_H
#define TOKENS_H

//#include <stdlib.h>
//#include <stdio.h>

/* token types */

enum {
	TOK_IF = 0,
	TOK_WHILE,
	TOK_INT,
	TOK_ECHO,
	TOK_DRAW,
	TOK_WAIT,
	TOK_CMX,
	TOK_CMY,
	TOK_MX,
	TOK_MY,
	TOK_OD,
	TOK_INTEGER,
	TOK_PLUS,
	TOK_MINUS,
	TOK_DIV,
	TOK_MUL,
	TOK_ASGN,
	TOK_EQ,
	TOK_GT,
	TOK_LT,
	TOK_GTE,
	TOK_LTE,
	TOK_NEQ,
	TOK_PLUSEQ,
	TOK_MINUSEQ,
	TOK_DIVEQ,
	TOK_MULEQ,
	TOK_WHITESPACE,
	TOK_IDENT,
	TOK_PLUSPLUS,
	TOK_MINUSMINUS,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_SEMICOLON,
	TOK_ELSE,
	TOK_COMMA,
	TOK_NEWLINE,
	TOK_GOTO,
	TOK_COLON,
	TOK_LBRACK,
	TOK_RBRACK,
	C_CMNT_OPEN,
	C_CMNT_CLOSE,
	CPP_CMNT,
	TOK_PROC,
	TOK_RET,
	TOK_FOR,
	TOK_QUOT,
	TOK_NAN,
	TOK_MNUM,
	TOK_MVAR,
	TOK_DOLLAR,
	TOK_CC,
	TOK_EXP,
	TOK_NOTSYMBOL,
	TOK_PIPE
};


/* routines */

int is_add_op(char type);
int is_asg_op(char type);
int is_mul_op(char type);
int is_comp_op(char type);
int is_instr(char type);
void tok_display(void *f, token_t t);
token_t* make_fake_tok(char *s);

#endif

#ifndef TREE_H
#define TREE_H

//#include <stdlib.h>

enum {		/* head_type */
	BLOCK = 0,
	ADD = 1,
	SUB = 2,
	MULT = 3,
	DIV = 4,
	ASGN = 5,
	IF = 6,
	WHILE = 7,
	NUMBER = 8,
	VARIABLE = 9,
	GT = 10,
	LT = 11,
	GTE = 12,
	LTE = 13,
	EQL = 14,
	NEQL = 15,
	INT_DECL = 16,
	BPF_INSTR = 17,
	NEGATIVE = 18,
	INC = 19,
	DEC = 20,
	POST_INC = 21,
	POST_DEC = 22,
	LABEL = 23,
	GOTO = 24,
	ARRAY = 25,
	ARRAY_DECL = 26,
	ARG_LIST = 27,
	PROC = 28,
	RET = 29,
	FUNC = 30,
	SYMBOL = 31,
	MATCH_NUM = 32,
	MATCH_VAR = 33,
	MATCH_IND = 34,
	MATCH_CONT = 35,
	MATCH_FUNC = 36,
	MATCH_CONTX = 37,
	MATCH_NAN = 38,
	MATCH_IND_SYM = 39,
	EXP = 40,
	/* special */
	NULL_TREE = 41
};

typedef struct exp_tree {
	char head_type;
	/* token_t* tok; */
	struct token* tok;
	int child_count;
	int child_alloc;
	struct exp_tree **child;
} exp_tree_t;

exp_tree_t null_tree;

void add_child(exp_tree_t *dest, exp_tree_t* src);
exp_tree_t new_exp_tree(int type, token_t* tok);
int valid_tree(exp_tree_t et);
exp_tree_t *alloc_exptree(exp_tree_t et);
void printout_tree(exp_tree_t et);
exp_tree_t* copy_tree(exp_tree_t *);
int is_special_tree(int ht);

#endif

#ifndef PARSER_H
#define PARSER_H

//#include "tree.h"

exp_tree_t parse(token_t *t);

#endif


#ifndef DICT_H
#define DICT_H

//#include "tree.h"

typedef struct dict_s {
	char** symbol;
	exp_tree_t **match;
	int count;
	int alloc;
	int success;
} dict_t;

//dict_t* new_dict();
void expand_dict(dict_t* d, int alloc);
int dict_search(dict_t *d, char* var);
void printout_dict(dict_t *d);

#endif



/* We recursively iterate through all 
 * rules on all the children of the tree
 * until the expression is irreducible, 
 * i.e. stops changing.
 * 
 * Also, we (try to) apply the optimization
 * routine.
 */

int apply_rules_and_optimize(exp_tree_t** rules, int rc, exp_tree_t *tree)
{
	int success = 0;
	while (1) {
		if (matchloop(rules, rc, tree)) {
			/*
			 * Reduction algorithm
			 * suceeded, print reduced tree
			 */

			success = 1;
		}

		/*
		 * If optimization succeeds in
		 * modifying the tree, try
		 * reducing the new optimized
		 * tree.
		 */
		if (optimize(tree)) {

			success = 1;
			continue;
		}
		break;
	}
	return success;
}
//#include "gc.h"

/*
 * Pretty compiler-failure error.
 * (same code as in my wannabe compiler
 * from december 2012, from which this
 * was taken from)
 */

//#include "tokenizer.h"
//#include "tokens.h"
//#include <string.h>

void compiler_fail(char *message, token_t *token,
        int in_line, int in_chr)
{
	char buf[1024];
	int line;
	int chr;
	int i;

	/* check for specific line/chr override
	 * (used for special cases like end of line) */
	line = in_line ? in_line : token->from_line;
	chr = in_line ? in_chr : token->from_char;

	fflush(stdout);
	fflush(stderr);

	/* copy the originating code line to a buffer,
	 * then print this to stderr */
	strncpy(buf, code_lines[line], 1024);
	buf[1023] = 0;
	for (i = 0; code_lines[line]; ++i)
		if (buf[i] == '\n') {
			buf[i] = 0;
			break;
		}
	fprintf(stderr, "%s\n", buf);

	/* put a little arrow under the offending
	 * character */
	for (i = 1; i < chr; ++i)
		if (buf[i - 1] == '\t')
			fputc(buf[i - 1], stderr);
		else
			fputc(' ', stderr);
	fputc('^', stderr);
	fputc('\n', stderr);

	/* finally display line number and error
	 * description */
	fprintf(stderr, "line %d: %s\n",
		line,
		message);
	exit(1);
}
//#include "gc.h"

/*
 * Data structure: symbol-matches dictionary produced
 * by the matching algorithm and used by the substitution
 * algorithm.
 */

//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>
//#include "dict.h"

void fail(char*);

dict_t* new_dict()
{
	int i;

	dict_t* d = cgc_malloc(sizeof(dict_t));
	d->symbol = cgc_malloc(100 * sizeof(char *));
	for (i = 0; i < 100; ++i)
		d->symbol[i] = cgc_malloc(128);
	d->match = cgc_malloc(100 * sizeof(exp_tree_t *));
	if (!d->symbol || !d->match)
		fail("cgc_malloc dict");
	d->count = 0;
	d->alloc = 100;
	d->success = 0;
	return d;
}

void expand_dict(dict_t* d, int alloc)
{
	int i;
	int old = d->alloc;
	d->alloc = alloc;
	d->symbol = realloc(d->symbol, d->alloc * sizeof(char *));
	for (i = old; i < alloc; ++i)
		d->symbol[i] = cgc_malloc(128);
	d->match = realloc(d->match, d->alloc * sizeof(exp_tree_t *));
}

int dict_search(dict_t *d, char* var)
{
	int i;
	for (i = 0; i < d->count; ++i)
		if (!strcmp(d->symbol[i], var))
			return i;
	return -1;
}

/* for debugging purposes */
void printout_dict(dict_t *d)
{
	int i =0;
	printf("[");
	for (i = 0; i < d->count; ++i) {
		if (i)
			printf(", ");
		printf("%s: ", d->symbol[i]);
		fflush(stdout);
		
		fflush(stdout);
	}
	printf("]");
	fflush(stdout);
}


/*
 * rough "garbage collector" helping system
 * (memory allocations are made into groups
 * which are freed by the main code when it
 * is clear that said memory is done with)
 */

//#include "gc.h"
//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <stddef.h>

 gcarray_t* curr = NULL;

void add_to_pile(gcarray_t *gca, void *ptr)
{
	if (!gca)
		return;
	if (++(gca->memb) > gca->alloc) {
		gca->alloc += 32;
		if (!(gca->pile = realloc(gca->pile, gca->alloc * sizeof(void *))))
			fail("out of memory");
	}
	gca->pile[gca->memb - 1] = ptr;
}

void *cgc_malloc(long size)
{
	void *ptr = malloc(size);
	add_to_pile(curr, ptr);
	return ptr;
}

void cgc_set(gcarray_t *gca)
{
	curr = gca;
}

gcarray_t* new_gca()
{
	gcarray_t *g = malloc(sizeof(gcarray_t));
	if (!g)
		fail("out of memory");
	g->pile = malloc(8 * sizeof(void *));
	if (!(g->pile))
		fail("out of memory");
	g->memb = 0;
	g->alloc = 8;
	return g;
}

void gca_cleanup(gcarray_t *gca)
{
	int i;

	for (i = 0; i < gca->memb; ++i)
		free(gca->pile[i]);

	free(gca->pile);
	free(gca);
}


//#include "gc.h"

/* General-use routines */

//#include <stdlib.h>
//#include <stdio.h>

void fail(char* mesg)
{
	fprintf(stderr, "error: %s\n", mesg);
	exit(1);
}

void sanity_requires(int exp)
{
	if(!exp)
		fail("that doesn't make any sense");
}

//#include "gc.h"

/*
 * Print out an expression tree in standard infix notation,
 * e.g. 1 + 2 * 3, omitting unnecessary parentheses.
 */

//#include "tree.h"
//#include "tokenizer.h"
//#include "tokens.h"
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>

char name[128];

/*
 * Helper routine: check precedence of an
 * operator. (Used by the unnecessary-
 * parenthesis-omission code below).
 */
int op_prec(int head_type)
{
	switch (head_type) {
		case EQL:	return 1;
		case EXP:	return 2;
		case DIV:	return 3;
		case MULT:	return 3;
		case SUB:	return 4;
		case ADD:	return 4;
		default:	return 0;
	}
}

/* Helper: tree type -> operator symbol */
char opsym(int head_type)
{
	switch (head_type) {
		case MULT:	return '*';
		case SUB:	return '-';
		case ADD:	return '+';
		case DIV:	return '/';
		case EQL:	return '=';
		case EXP:	return '^';
	}
}

void printout_tree_infix(exp_tree_t* pet)
{
	void printout_tree_infix_derp(exp_tree_t* pet, int pp);
	printout_tree_infix_derp(pet, 0);
}

void printout_tree_infix_derp(exp_tree_t* pet, int pp)
{
	int i;
	int parens_a = 0;
	int parens_b = 0;
	exp_tree_t et;

	memcpy(&et, pet, sizeof(exp_tree_t));

	/* 'foo(bar, donald, baker)' */
	if (et.head_type == FUNC) {
		strncpy(name, (et.tok)->start, (et.tok)->len);
		name[(et.tok)->len] = 0;
		printf("%s(", name);
		fflush(stdout);
		for (i = 0; i < et.child_count; i++) {
			if (i) {
				printf(", ");
				fflush(stdout);
			}
			printout_tree_infix_derp(et.child[i], 0);
		}
		printf(")");
		fflush(stdout);
		return;
	}

	/* 'bar' */
	/* '123' */
	if (et.head_type == VARIABLE
	|| et.head_type == NUMBER) {
		strncpy(name, (et.tok)->start, (et.tok)->len);
		name[(et.tok)->len] = 0;
		printf("%s", name);
		fflush(stdout);
		return;
	}

	/* '-X' */
	if (et.head_type == NEGATIVE) {
		printf("-");
		fflush(stdout);
		printout_tree_infix_derp(*et.child, 0);
		return;
	}

	/* 'A + B + C' */
	if (et.head_type == MULT
	|| et.head_type == ADD
	|| et.head_type == SUB
	|| et.head_type == DIV
	|| et.head_type == EQL
	|| et.head_type == EXP) {
		/* 
		 * Need parentheses if child has lower precedence.
		 * e.g. (1 + 2) * 3
		 */
		parens_a = pp && pp < op_prec(et.head_type);
		if (parens_a) {
			printf("(");
			fflush(stdout);
		}

		for (i = 0; i < et.child_count; i++) {
			/* 
			 * Check for the second kind of need-of-parentheses,
			 * for example in expressions like 1 / (2 / 3) 
			 */
			parens_b = op_prec(et.head_type) == 
					op_prec(et.child[i]->head_type)
			    && et.child[i]->child_count
				&& op_prec(et.head_type)
				&& et.head_type != MULT
				&& et.head_type != ADD;

			if (parens_b)
				printf("(");

			/* print child */
			printout_tree_infix_derp(et.child[i], op_prec(et.head_type));

			if (parens_b) {
				printf(")");
				fflush(stdout);
			}

			/* 
			 * In infix notation, an operator symbol comes
			 * after every child except the last
			 */
			if (i < et.child_count - 1) {
				printf(" %c ", opsym(et.head_type));
				fflush(stdout);
			}
		}

		/* close first type of parentheses-need */
		if (parens_a) {
			printf(")");
			fflush(stdout);
		}
		return;
	}
}


//#include "gc.h"

/*
==============================================================================
        This is a symbolic differentiation program

   It features an infix REPL, a tree pattern-matcher/substitutor,
   and some preloaded derivative rules (the fanciest of which is
   probably the chain rule)

   This is a toy/self-education project by "blockeduser", May 2013.
   It is very far from perfect and could be improved upon.
   It is coded in hopefully-not-too-bad C.

   Much of it is strongly based on examples given in the SICP
   ("Structure and Interpretation of Computer Programs") book and videos
==============================================================================
*/

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include "tokenizer.h"
//#include "tokens.h"
//#include "tree.h"
//#include "parser.h"
//#include "dict.h"
//#include "subst.h"
//#include "optimize.h"
//#include "match.h"
//#include "infix-printer.h"
//#include "apply-rules.h"
//#include "rulefiles.h"
//#include "gc.h"

int main(int argc, char** argv)
{
	token_t* tokens;
	exp_tree_t* tree;
	int i;
	exp_tree_t* rules[128];
	exp_tree_t* pres_rules[128];
	int rc;
	int prc;
	char lin[1024];
	gcarray_t *setup, *iter;
	exp_tree_t et;

	void fail(char*);
	int unwind_expos(exp_tree_t *et);

#ifdef LEAK_STRESS_TEST
	strcpy(lin, "diff(z^z^z, z)");
#endif

	cgc_set(setup = new_gca());

	/*
	 * Setup lexer (wannabe regex code)
	 */
	setup_tokenizer();

	/*
	 * Read rules stored in "rules"
	 * directory. Boring file processing
	 * stuff that calls the parser and
 	 * stores parsed trees into an array.
	 * Code is data. Data is code.
	 */
	rc = readrules(rules, "rules");

	/* There are also rules for presentation
	 * of the final result, for example
	 * the notation e^x is preferred to
	 * the notation exp(x), etc.
	 */
	prc = readrules(pres_rules, "pres-rules");

	printf("Welcome to the REPL.\n");
	printf("As always, ctrl-C to exit.\n");

	while (1) {

#ifndef LEAK_STRESS_TEST
		/* Print prompt, read line */
		printf("]=> ");
		fflush(stdout);
		if (!fgets(lin, 1024, stdin)) {
			printf("\n");	/* (stops gracefully on EOF) */
			break;
		}
#endif

		/* Skip empty lines. The parsing code really
		 * hates them, and by "really" I mean segfault. */
		if (!lin[1])
			continue;

		/* Check for command (!xxx) */
		if (*lin == '!') {
			/* !sr: show stored rules */
			if (!strcmp(lin, "!sr\n")) {
				printf("%d rule%s memorized%s\n",
					rc,
					rc > 1 ? "s have been" : " has been",
					rc ? ":" : "");
				for (i = 0; i < rc; ++i) {
					printf("%d: ", i);
					fflush(stdout);
					
					putchar('\n');
				}
			}
			/* Don't parse this ! */
			continue;
		}

		cgc_set(iter = new_gca());

		/* Lex the line */
		tokens = tokenize(lin);



		/* Parse the tokens into an expression-tree */
		et = parse(tokens);
		tree = et.child[0];




		/*
		 * If the expression is an equation,
		 * it's considered a pattern-matching rule
		 * and gets added to the list of rules.
		 */
		if (tree->head_type == EQL) {
			rules[rc++] = alloc_exptree(*tree);
			printf("Rule stored.\n");
		} else {
			/*
			 * If, on the other hand, the expression
			 * is not an equation, well we try to
			 * reduce it using the rules.
			 */

			/* Print final result if any reductions
			 * succeeded */
			if (apply_rules_and_optimize(rules, rc, tree)) {
				/* Some final clean-up ... */
				while(unwind_expos(tree))
					;
				(void)apply_rules_and_optimize(pres_rules, prc,
					tree);
#ifndef LEAK_STRESS_TEST
				printout_tree_infix(tree);
				printf("\n");
#endif
			}
		}

		gca_cleanup(iter);
	}

	gca_cleanup(setup);

	return 0;
}
//#include "gc.h"

/*
 * Tree pattern-matching algorithm,
 * based on the one described in the SICP video
 * about pattern-matching ("4A: Pattern Matching and 
 * Rule-based Substitution").
 *
 * The nasty extensions are not from SICP.
 */

//#include "tree.h"
//#include "tokenizer.h"
//#include "tokens.h"
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>
//#include "dict.h"
//#include "subst.h"
//#include "infix-printer.h"


void fail(char* mesg);
int sametree(exp_tree_t *a, exp_tree_t *b);



/* Is "tree" not identical to "key"
 * or a father (recursively) of such a tree ?
 *
 * This is used for special pattern-matching
 * syntaxes like e:x and e|x, which only match
 * an expression when it contains (or not)
 * a chosen sub-expression.
 */
int indep(exp_tree_t *tree, exp_tree_t *key)
{
	int i;

	if(sametree(tree, key))
		return 0;

	for (i = 0; i < tree->child_count; ++i) {
		if (sametree(tree->child[i], key))
			return 0;
		if (tree->child[i]->child_count)
			if(!indep(tree->child[i], key))
				return 0;
	}

	return 1;
}

exp_tree_t* find_subtree(exp_tree_t *tree, exp_tree_t *key)
{
	int i;
	exp_tree_t *catch;

	if(sametree(tree, key))
		return tree;

	for (i = 0; i < tree->child_count; ++i) {
		if (sametree(tree->child[i], key))
			return tree->child[i];
		if (tree->child[i]->child_count)
			if((catch = find_subtree(tree->child[i], key)) != NULL)
				return catch;
	}

	return NULL;
}

int same_tokens(exp_tree_t *a, exp_tree_t *b)
{
	strncpy(buf_a, b->tok->start, b->tok->len);
	buf_a[b->tok->len] = 0;

	strncpy(buf_b, a->tok->start, a->tok->len);
	buf_b[a->tok->len] = 0;

	return !strcmp(buf_a, buf_b);
}

/* 
 * This routine tries to match the tree 'a' against
 * the pattern 'b'.
 * 
 * If the routine returns true (1), the dictionary 'd'
 * has been populated with the symbol-mappings for the
 * match.
 */ 
int treematch(exp_tree_t *a, exp_tree_t *b, dict_t* d)
{
	int i;
	int id;

	/* 
	 * VARIABLE: general (any tree) match, 
	 * MATCH_VAR: variable match,
	 * MATCH_NUM: number match
	 * MATCH_NAN: match if not number
	 * SYMBOL: match exact variable name
	 * MATCH_IND_SYM: match if not exact variable name
	 */
	if (b->head_type == VARIABLE
	|| (b->head_type == MATCH_NUM && a->head_type == NUMBER)
	|| (b->head_type == MATCH_VAR && a->head_type == VARIABLE)
	|| (b->head_type == MATCH_NAN && a->head_type != NUMBER)
	|| (b->head_type == SYMBOL && a->head_type == VARIABLE
		&& same_tokens(a, b))) {
		strncpy(buf_a, b->tok->start, b->tok->len);
		buf_a[b->tok->len] = 0;
		if ((id = dict_search(d, buf_a)) != -1) {
			if (!sametree(d->match[id], a))
				return 0;
			return 1;
		} else {
			strcpy(d->symbol[d->count], buf_a);
			d->match[d->count] = a;
			if (++d->count > d->alloc)
				expand_dict(d, d->count + 10);
			return 1;
		}
	}

	/*
	 * (VARIABLE:bar)
	 * (MATCH_IND_SYM (VARIABLE:a) (VARIABLE:e))
	 * Match anything that isn't a given symbol
	 */
	if (b->head_type == MATCH_IND_SYM) {
		if (a->head_type != VARIABLE) {
			strncpy(buf_a, b->child[0]->tok->start, b->child[0]->tok->len);
			buf_a[b->child[0]->tok->len] = 0;
			strcpy(d->symbol[d->count], buf_a);
			d->match[d->count] = a;
			if (++d->count > d->alloc)
				expand_dict(d, d->count + 10);
			return 1;
		} else {
			if (same_tokens(a, b->child[1]))
				return 0;
			else {
				strncpy(buf_a, b->child[0]->tok->start, b->child[0]->tok->len);
				buf_a[b->child[0]->tok->len] = 0;
				strcpy(d->symbol[d->count], buf_a);
				d->match[d->count] = a;
				if (++d->count > d->alloc)
					expand_dict(d, d->count + 10);
				return 1;
			}
		}
	}

	/* (MATCH_XXX (VARIABLE:e) (VARIABLE:x)) ...
	 * 
	 * MATCH_IND: match "E" if it contains no occurence of the symbol
	 * stored for "X"
 	 *
	 * MATCH_CONT: match "E" if it contains an occurence of, or is directly
	 * the symbol stored for "X"
	 *
	 * MATCH_CONTX: match "E" if it contains an occurence of, and isn't
	 * directly the symbol stored for "X"
	 */
	if (b->head_type == MATCH_IND
		|| b->head_type == MATCH_CONT
		|| b->head_type == MATCH_CONTX) {
		/* find symbol stored for "X" */
		strncpy(buf_a, b->child[1]->tok->start, b->child[1]->tok->len);
		buf_a[b->child[1]->tok->len] = 0;
		if ((id = dict_search(d, buf_a)) != -1) {
			/* is this expression indepedent of the symbol ? */
			/* (or non-indepedent, etc) */
			if ((b->head_type == MATCH_IND
			    && indep(a, d->match[id]))
			|| (b->head_type == MATCH_CONT
			    && !indep(a, d->match[id]))
			|| (b->head_type == MATCH_CONTX
			    && !indep(a, d->match[id])
			    && !sametree(a, d->match[id]))) {
				/* yes, do standard symbolmatching */
				strncpy(buf_a, b->child[0]->tok->start,
					b->child[0]->tok->len);
				buf_a[b->child[0]->tok->len] = 0;
				if ((id = dict_search(d, buf_a)) != -1) {
					if (!sametree(d->match[id], a))
						return 0;
					return 1;
				} else {
					strcpy(d->symbol[d->count], buf_a);
					d->match[d->count] = a;
					if (++d->count > d->alloc)
						expand_dict(d, d->count + 10);
					return 1;
				}
			}
		} else {
			/* Problematic situation for which there
			 * may be a remedy... In other words,
			 * probably-fixable bug.
			 *
			 * See what "SYNTAX" file says about bug with
			 * e|x and e:x syntax.
			 */
			printf("[forward substitution issue]\n");
			return 0;
		}
	}

	/* (MATCH_FUNC:foo (ADD (VARIABLE:x) (NUMBER:1)))
	 * 
	 * MATCH_FUNC: match a function and then do standard
	 * matching on its children
	 */
	if (b->head_type == MATCH_FUNC && a->head_type == FUNC) {
		/* get meta-symbol */
		strncpy(buf_a, b->tok->start, b->tok->len);
		buf_a[b->tok->len] = 0;

		/* find function name in pattern */
		strncpy(buf_b, a->tok->start, a->tok->len);
		buf_b[a->tok->len] = 0;

		/* already in dictionary ? */
		if ((id = dict_search(d, buf_a)) != -1) {
			/* same function name ? */
			strncpy(buf_c, d->match[id]->tok->start,
				d->match[id]->tok->len);
			buf_c[d->match[id]->tok->len] = 0;
			if (strcmp(buf_c, buf_b))
				return 0;	/* inconsistent */
		} else {
			/* not in dictionary, add it */
			strcpy(d->symbol[d->count], buf_a);
			d->match[d->count] = a;
			if (++d->count > d->alloc)
				expand_dict(d, d->count + 10);
		}

		/* don't forget to match the children !!! */
		if (a->child_count != b->child_count)
			return 0;
		for (i = 0; i < a->child_count; ++i) {
			if (!treematch(a->child[i], b->child[i], d))
				return 0;
		}
		return 1;
	}

	/* trivial non-matching cases */
	if (a->head_type != b->head_type)
		return 0;

	if (a->child_count != b->child_count)
		return 0;

	/* compare token-strings */
	if (a->tok && b->tok) {
		strncpy(buf_a, a->tok->start, a->tok->len);
		buf_a[a->tok->len] = 0;
		strncpy(buf_b, b->tok->start, b->tok->len);
		buf_b[b->tok->len] = 0;
		if (strcmp(buf_a, buf_b))
			return 0;
	}

	/* recurse onto children */
	for (i = 0; i < a->child_count; ++i) {
		if (!treematch(a->child[i], b->child[i], d))
			return 0;
	}

	return 1;
}

/*
 * Recursively iterate through all rules on all the
 * children of the tree until the
 * expression is irreducible, i.e. stops
 * changing.
 */
int matchloop(exp_tree_t** rules, int rc, exp_tree_t* tree)
{
	dict_t *dict;
	exp_tree_t *skel;
	int i;
	int mc;
	int m = 0;

restart:
	doloop:
		mc = 0;
		
		/* Make a dictionary */
		dict = new_dict();

		/* Go through the rules */
		for (i = 0; i < rc; ++i) {

			/* Empty dictionary if necessary */
			if (dict->count)
				dict->count = 0;

			if (treematch(tree, rules[i]->child[0], dict)) {
				/* It matches. Do the correspoding substitution
				 * and make a little printout of the form:
				 * [rule number] original tree -> substituted
				 *				  tree
				 */
				++mc;
				m = 1;



				if (dict->count) {
					/* Substitution with symbols */
					skel = copy_tree(rules[i]->child[1]);
					subst(skel, dict, &skel);
					*tree = *skel;
				} else {
					/* Plain direct substitution */
					*tree = *copy_tree(rules[i]->child[1]);
				}




				break;
			}
		}
	/* If any of the rules matched, try them all again. */
		if (mc)
			goto doloop;

	/* Do matching loop on all children. If it succeeds,
	 * do the loop again on this tree. */
	for (i = 0; i < tree->child_count; ++i)
		if (matchloop(rules, rc, tree->child[i])) {
			m = 1;
			/* goto restart; */
		}
	return m;
}

//#include "gc.h"

/*
 * Optimizations:
 *
 * - constant folding for simple arithmetic
 * - canonicalization (flatten nesting)
 * - some algebraic simplifications
 *   (which get quite ugly to code)
 *
 * It has come at a point where almost
 * the bulk of the complete program is in
 * algebraic simplifications.
 *
 * Maybe it would be possible to come up with
 * a domain-language that simplifies the coding
 * of the algebraic simplifications...
 */

//#include "tree.h"
//#include "tokens.h"
//#include <stdio.h>
//#include <string.h>	/* memcpy */


long tok2int(token_t *t);
int sametree(exp_tree_t *a, exp_tree_t *b);


exp_tree_t new, *new_ptr;
exp_tree_t new2, *new2_ptr;
exp_tree_t num, *num_ptr;
exp_tree_t *cancel;

void make_tree_number(exp_tree_t *t, int n)
{
	char buffer[128];
	sprintf(buffer, "%d", n);
	t->tok = make_fake_tok(buffer);
	t->head_type = NUMBER;
	t->child_count = 0;
}

exp_tree_t* new_tree_number(int n)
{
	exp_tree_t t = new_exp_tree(NUMBER, NULL);
	make_tree_number(&t, n);
	return alloc_exptree(t);
}

/* check for an arithmetic operation tree */
int arith_type(int ty)
{
	return 	ty == ADD
			|| ty == SUB
			|| ty == MULT
			|| ty == DIV;
}

/*
 * Performs the conversion:
 * e^(a + b + c + ...) => e^a * e^b * e^c * ...
 */
int unwind_expos(exp_tree_t *et)
{
	int i, q;
	int res = 0;
	exp_tree_t *derp, *below;

	for (i = 0; i < et->child_count; ++i)
		res |= unwind_expos(et->child[i]);
			
	if (et->head_type == EXP
		&& et->child_count == 2
		&& et->child[1]->head_type == ADD
		&& et->child[1]->child_count >= 2) {

		if (et->child[0]->head_type == VARIABLE) {
			strncpy(buf, et->child[0]->tok->start, et->child[0]->tok->len);
			buf[et->child[0]->tok->len] = 0;

			if (!strcmp(buf, "e")) {
				/* yes */

				/* derp = e^  */
				derp = copy_tree(et);
				derp->child_count = 1;

				new = new_exp_tree(MULT, NULL);
				new_ptr = alloc_exptree(new);
				
				below = et->child[1];
				for (q = 0; q < below->child_count; ++q) {
					new2_ptr = copy_tree(derp);
					add_child(new2_ptr, below->child[q]);
					add_child(new_ptr, new2_ptr);
				}

				/* transpose */
				memcpy(et, new_ptr, sizeof(exp_tree_t));

				return 1;
			}
		}
	}
	
	return res;
}

int optimize(exp_tree_t *et)
{
	int i = 0;
	int q, w, e, r, t, y;
	int did = 0;
	int chk, chk2, chk3;
	int a, b;
	int val;
	exp_tree_t *below;
	exp_tree_t *herp;
	exp_tree_t *derp;
	exp_tree_t *right_child;
	token_t* make_fake_tok(char *s);

	for (i = 0; i < et->child_count; i++) {
		did |= optimize(et->child[i]);
	}

	/* e.g. (DIV 6 3) -> (NUMBER 2) */
	if (et->head_type == DIV
	&& et->child_count == 2
	&& et->child[0]->head_type == NUMBER
	&& et->child[1]->head_type == NUMBER) {
		a = tok2int(et->child[0]->tok);
		b = tok2int(et->child[1]->tok);
		if (b != 0 && a % b == 0) {
			et->child_count = 0;
			et->head_type = NUMBER;
			sprintf(buf, "%d", a / b);
			et->tok = (token_t *)make_fake_tok(buf);
			return(1);
		}
	}

	/*
	 * Fold trivial arithmetic constants
	 */
	if (et->head_type == MULT
	|| et->head_type == ADD
	|| et->head_type == SUB
	|| et->head_type == NEGATIVE) {
		chk = 1;
		for (i = 0; i < et->child_count; ++i)
			if (et->child[i]->head_type != NUMBER) {
				chk = 0;
				break;
			}

		if (chk) {
			did = 1;
			val = tok2int(et->child[0]->tok);
			for (i = 1; i < et->child_count; ++i) {
				switch(et->head_type) {
					case MULT:
					val *= tok2int(et->child[i]->tok);
					break;

					case ADD:
					val += tok2int(et->child[i]->tok);
					break;

					case SUB:
					val -= tok2int(et->child[i]->tok);
					break;
				}
			}

			if (et->head_type == NEGATIVE)
				val *= -1;

			et->head_type = NUMBER;
			sprintf(buf, "%d", val);
			et->tok = (token_t *)make_fake_tok(buf);
			et->child_count = 0;
		} else {
			/* 
			 * Simplify mixed number/non-number
			 * to one number and the rest 
			 */
			chk = 0;
			chk2 = 0;
			for (i = 0; i < et->child_count; ++i) {
				if (et->child[i]->head_type == NUMBER)
					chk++;
				if (et->child[i]->head_type != NUMBER)
					chk2 = 1;
			}

			if ((chk > 1 || (chk == 1 && et->child[0]->head_type != NUMBER))
				&& chk2
				&& (et->head_type == ADD || et->head_type == MULT)) {
				new = new_exp_tree(et->head_type, et->tok);
				val = et->head_type == ADD ? 0 : 1;
				for (i = 0; i < et->child_count; ++i) {
					if (et->child[i]->head_type == NUMBER) {
						switch(et->head_type) {
							case MULT:
							val *= tok2int(et->child[i]->tok);
							break;

							case ADD:
							val += tok2int(et->child[i]->tok);
							break;
						}
					}
				}
				sprintf(buf, "%d", val);
				num = new_exp_tree(NUMBER, make_fake_tok(buf));
				new_ptr = alloc_exptree(new);
				add_child(new_ptr, alloc_exptree(num));
				
				for (i = 0; i < et->child_count; ++i)
					if (et->child[i]->head_type != NUMBER)
						add_child(new_ptr, copy_tree(et->child[i]));

				/* overwrite tree data */
				memcpy(et, new_ptr, sizeof(exp_tree_t));

				return(1);
			}
		}
	}

	/* Simplify nested binary trees,
	 * e.g. (SUB (SUB (NUMBER:3) (NUMBER:2)) (NUMBER:1))
	 * becomes (SUB (NUMBER:3) (NUMBER:2) (NUMBER:1))
	 */
	if (et->child_count == 2
		&& et->child[0]->head_type == et->head_type
		&& arith_type(et->head_type)) {
		below = et->child[0];
		right_child = et->child[1];
		et->child_count = 0;
		for (i = 0; i < below->child_count; i++)
			add_child(et, below->child[i]);
		add_child(et, right_child);
		return(1);
	}

	/* (MUL (MUL xxx) yyy zzz) => (MUL xxx yyy zzz) */
	if (et->child_count >= 2
		&& et->child[0]->head_type == et->head_type
		&& et->child[0]->child_count
		&& (et->head_type == MULT || et->head_type == ADD)) {

		new = new_exp_tree(MULT, NULL);
		new_ptr = alloc_exptree(new);

		below = et->child[0];	

		for (q = 0; q < below->child_count; ++q)
			add_child(new_ptr, below->child[q]);
			
		for (q = 1; q < et->child_count; ++q)
			add_child(new_ptr, et->child[q]);

		memcpy(et, new_ptr, sizeof(exp_tree_t));
		
		return(1);
	} 

	/* (MUL xxx (MUL yyy)) => (MUL xxx yyy) */
	if (et->child_count >= 2
		&& et->child[et->child_count - 1]->head_type
		== et->head_type
		&& (et->head_type == MULT || et->head_type == ADD)) {
		below = et->child[et->child_count - 1];
		if (below->child_count) {
			--(et->child_count);
			for (i = 0; i < below->child_count; i++)
				add_child(et, below->child[i]);
			return(1);
		}
	}

	/* (A * 123 * C) / 456 => (123/456) * A * C */
	if (et->child_count == 2
		&& et->head_type == DIV
		&& et->child[0]->head_type == MULT
		&& et->child[1]->head_type == NUMBER) {
		below = et->child[0];
		for (i = 0; i < below->child_count; ++i) {
			if (below->child[i]->head_type == NUMBER) {
				new = new_exp_tree(DIV, NULL);
				new_ptr = alloc_exptree(new);
				add_child(new_ptr, copy_tree(below->child[i]));
				add_child(new_ptr, copy_tree(et->child[1]));
				below->child[i] = new_ptr;
				et->child_count--;
				return(1);
			}
		}
	}

	/* (A * B * C) / C => A * B */
	if (et->child_count == 2
		&& et->head_type == DIV
		&& et->child[0]->head_type == MULT) {		
		below = et->child[0];
		for (i = 0; i < below->child_count; ++i) {
			if (sametree(below->child[i], et->child[1])) {
				et->child_count--;
				make_tree_number(below->child[i], 1);
				return(1);
			}
		}
	}

	/* 
	 * (A * B * C) / (X * Y * B * Z)
	 * = (A * C) / (X * Y * Z)
	 */
	if (et->child_count == 2
		&& et->head_type == DIV
		&& et->child[0]->head_type == MULT
		&& et->child[1]->head_type == MULT) {
		
		for (q = 0; q < et->child[0]->child_count; ++q) {
			cancel = et->child[0]->child[q];
			for (e = 0; e < et->child[1]->child_count; ++e) {
				below = et->child[1]->child[e];
				if (sametree(below, cancel)) {
					make_tree_number(below, 1);
					make_tree_number(cancel, 1);
					return(1);
				}
			}
		}
	}

	/* A * B * A * XYZ * A => A^3 * B * XYZ */
	if (et->child_count >= 2
		&& et->head_type == MULT) {
		for (q = 0; q < et->child_count; ++q) {
			chk = 0;
			for (w = 0; w < et->child_count; ++w) {
				if (sametree(et->child[w], et->child[q]))
					++chk;
				if (et->child[w]->head_type == EXP
				&& et->child[w]->child_count == 2
				&& sametree(et->child[w]->child[0], et->child[q])
				&& et->child[w]->child[1]->head_type == NUMBER)
					chk += tok2int(et->child[w]->child[1]->tok);
			}

			if (et->child[q]->head_type == NUMBER)
				chk = 0;

			if (chk > 1 || chk < 0) {
				chk2 = 0;
				cancel = copy_tree(et->child[q]);
				for (q = 0; q < et->child_count; ++q) {
					if (sametree(cancel, et->child[q])
					|| (et->child[q]->head_type == EXP
						&& et->child[q]->child_count == 2
						&& sametree(et->child[q]->child[0], cancel))) {
						if(!chk2) {
							chk2 = 1;
							new = new_exp_tree(EXP, NULL);
							new_ptr = alloc_exptree(new);
							add_child(new_ptr, cancel);
							add_child(new_ptr, new_tree_number(chk));
							et->child[q] = new_ptr;
						} else {
							make_tree_number(et->child[q], 1);
						}
					}
				}
				return(1);
			}
		}
	}

	/* A  + B + A + XYZ + A => A*3 + B + XYZ */
	if (et->child_count >= 2
		&& et->head_type == ADD) {
		for (q = 0; q < et->child_count; ++q) {
			chk = 0;
			for (w = 0; w < et->child_count; ++w) {
				if (sametree(et->child[w], et->child[q]))
					++chk;
				if (et->child[w]->head_type == MULT
				&& et->child[w]->child_count == 2
				&& sametree(et->child[w]->child[0], et->child[q])
				&& et->child[w]->child[1]->head_type == NUMBER)
					chk += tok2int(et->child[w]->child[1]->tok);
			}

			if (et->child[q]->head_type == NUMBER)
				chk = 0;

			if (chk > 1) {
				chk2 = 0;
				cancel = copy_tree(et->child[q]);
				for (q = 0; q < et->child_count; ++q) {
					if (sametree(cancel, et->child[q])
					|| (et->child[q]->head_type == MULT
						&& et->child[q]->child_count == 2
						&& sametree(et->child[q]->child[0], cancel))) {
						if(!chk2) {
							chk2 = 1;
							new = new_exp_tree(MULT, NULL);
							new_ptr = alloc_exptree(new);
							add_child(new_ptr, cancel);
							add_child(new_ptr, new_tree_number(chk));
							et->child[q] = new_ptr;
						} else {
							make_tree_number(et->child[q], 0);
						}
					}
				}
				return(1);
			}
		}
	}

	/* 
	 * Remove 1's from products, 0's from additions,
	 * 0's from subtractions 
	 */
	if ((et->head_type == MULT
	|| et->head_type == ADD
	|| et->head_type == SUB)
	&& et->child_count >= 2) {
		chk = 0;
		q = et->head_type == MULT;
		for (i = 0; i < et->child_count; ++i) {
			if (et->child[i]->head_type == NUMBER
			&& tok2int(et->child[i]->tok) == q) {
				++chk;
			}
		}
		if (chk) {
			new = new_exp_tree(et->head_type, NULL);
			new_ptr = alloc_exptree(new);
			for (i = 0; i < et->child_count; ++i)
				if (!(et->child[i]->head_type == NUMBER
				&& tok2int(et->child[i]->tok) == q))
					add_child(new_ptr, copy_tree(et->child[i]));

			/* transpose */
			memcpy(et, new_ptr, sizeof(exp_tree_t));

			return(1);
		}
	}

	/* A + B * A => A * (1 + B) */
	if (et->child_count >= 2
		&& et->head_type == ADD) {
		chk = 0;
		for (i = 0; i < et->child_count; ++i)
			if (et->child[i]->head_type == MULT) {
				chk = 1;
				break;
			}
		if (chk) {
			/* iterate thru terms */
			for (q = 0; q < et->child_count; ++q) {
				cancel = et->child[q];
				/* try to cancel term q from another term */
				for (w = 0; w < et->child_count; ++w) {
					below = et->child[w];
					if (below->head_type == MULT) {
						for (e = 0; e < below->child_count; ++e) {
							if (sametree(cancel, below->child[e])) {
								/* there ! cancel factor from term */

								make_tree_number(below->child[e], 1);
								/* cancel identical terms => 1 */
								cancel = copy_tree(cancel);
								for (r = 0; r < et->child_count; ++r)
									if (sametree(et->child[r], cancel))
										make_tree_number(et->child[r], 1);
								/* make new product tree */
								new = new_exp_tree(MULT, NULL);
								new_ptr = alloc_exptree(new);
								add_child(new_ptr, cancel);
								add_child(new_ptr, copy_tree(et));
						
								/* transpose */
								memcpy(et, new_ptr, sizeof(exp_tree_t));
								return(1);
							}
						}
					}
				}
			}
		}
	}

	/* (DIV foo), (ADD foo), etc. -> foo */
	if (et->child_count == 1
	&& arith_type(et->head_type)) {
		below = et->child[0];
		memcpy(et, below, sizeof(exp_tree_t));
		return(1);
	}

	/* (A * B * C) + (X * Y * B) = B * [(A*C) + (X*Y)] */
	if (et->child_count >= 2
		&& et->head_type == ADD) {
		chk = 1;
		for (i = 0; i < et->child_count; ++i)
			if (et->child[i]->head_type != MULT) {
				chk = 0;
				break;
			}
		
		if (chk) {
			/* iterate through terms */
			for (q = 0; q < et->child_count; ++q) {
				/* iterate through factors of a term */
				for (w = 0; w < et->child[q]->child_count; ++w) {
					/* check that this factor occurs in all terms */
					chk = 1;
					cancel = copy_tree(et->child[q]->child[w]);
					for (e = 0; e < et->child_count; ++e) {
						chk2 = 0;
						for (r = 0; r < et->child[e]->child_count; ++r) {
							if (sametree(et->child[e]->child[r], cancel)) {
								chk2 = 1;
								break;
							}
						}
						chk &= chk2;
					}

					/* don't factor-out numbers */
					if (cancel->head_type == NUMBER)
						chk = 0;

					if (is_special_tree(cancel->head_type))
						chk = 0;

					if (chk) {
						/* factor => 1 */
						for (e = 0; e < et->child_count; ++e)
							for (r = 0; r < et->child[e]->child_count; ++r)
								if (sametree(et->child[e]->child[r], cancel))
									make_tree_number(et->child[e]->child[r], 1);
					
						/* make new product tree */
						new = new_exp_tree(MULT, NULL);
						new_ptr = alloc_exptree(new);
						add_child(new_ptr, cancel);
						add_child(new_ptr, copy_tree(et));
						
						/* transpose */
						memcpy(et, new_ptr, sizeof(exp_tree_t));

						return(1);
					}
				}
			}
		}
	}

	/* A * B * (C / D) = (A * B * C) / D */
	if (et->child_count > 2
		&& et->head_type == MULT
		&& et->child[et->child_count - 1]->head_type == DIV
		&& et->child[et->child_count - 1]->child_count == 2) {
		below = copy_tree(et->child[et->child_count - 1]);
		et->child_count--;
		add_child(et,
			below->child[0]);
		new = new_exp_tree(DIV, NULL);
		new_ptr = alloc_exptree(new);
		add_child(new_ptr, copy_tree(et));
		add_child(new_ptr, 
			below->child[1]);
		memcpy(et, new_ptr, sizeof(exp_tree_t));
		return(1);
	} 

	/* 
	 * (A * B * C ^ 2) / C ^ 6 
	 * => A * B * C ^ -4 
	 */
	if (et->child_count == 2
		&& et->head_type == DIV
		&& et->child[0]->head_type == MULT
		&& et->child[1]->head_type == EXP
		&& et->child[1]->child_count == 2) {

		cancel = copy_tree(et->child[1]->child[0]);
		below = et->child[0];

		chk = 0;
		for (i = 0; i < below->child_count; ++i) {
			if (below->child[i]->child_count == 2
			&& below->child[i]->head_type == EXP) {
				if (sametree(below->child[i]->child[0], cancel)) {
					chk = 1;
					new = new_exp_tree(SUB, NULL);
					new_ptr = alloc_exptree(new);
					add_child(new_ptr, copy_tree(below->child[i]->child[1]));
					add_child(new_ptr, copy_tree(et->child[1]->child[1]));
					below->child[i]->child[1] = new_ptr;
				}
			}
		}
		if (chk) {
			et->child_count--;
			return(1);
		}

	}

	/*
	 * sin(x) / (sin(x)^2 * cos(x))
	 * => 1 / (sin(x) * cos(x))
	 */
	if (et->head_type == DIV
		&& et->child_count == 2	
		&& et->child[1]->head_type == MULT) {

		cancel = et->child[0];
		below = et->child[1];
	
		for (i = 0; i < below->child_count; ++i) {
			if (below->child[i]->head_type == EXP
				&& below->child[i]->child_count == 2) {
					if (sametree(cancel, below->child[i]->child[0])) {
					
						num = new_exp_tree(SUB, NULL);
						num_ptr = alloc_exptree(num);
						add_child(num_ptr, new_tree_number(1));
						add_child(num_ptr, 
							copy_tree(below->child[i]->child[1]));

						new = new_exp_tree(EXP, NULL);
						new_ptr = alloc_exptree(new);
						add_child(new_ptr, et->child[0]);
						add_child(new_ptr, num_ptr);

						make_tree_number(below->child[i], 1);

						et->child[0] = new_ptr;
						return(1);
					}
			}
		}
	}

	/*
	 * x ^ 2 * x ^3 * x^A = x ^ (5 + A)
	 */
	if (et->head_type == MULT
		&& et->child_count >= 2) {
		for (i = 0; i < et->child_count; ++i) {
			if (et->child[i]->head_type == EXP
				&& et->child[i]->child_count == 2
				&& et->child[i]->child[0]->head_type == VARIABLE) {
				cancel = et->child[i]->child[0];

				/* look for other exponentiations in this variable */
				chk = 0;
				for (q = 0; q < et->child_count; ++q) {
					if (et->child[q]->head_type == EXP
						&& et->child[q]->child_count == 2
						&& et->child[q]->child[0]->head_type == VARIABLE
						&& sametree(cancel, et->child[q]->child[0])) {
						++chk;
					}
				}
			
				/* don't do this on e^x */
				strncpy(buf, cancel->tok->start, cancel->tok->len);
				buf[cancel->tok->len] = 0;
				if (!strcmp(buf, "e"))
					chk = 0;

				/* yes there are several. first sum up the exponents */
				if (chk > 1) {
					new = new_exp_tree(ADD, NULL);
					new_ptr = alloc_exptree(new);
					for (q = 0; q < et->child_count; ++q) {
						if (et->child[q]->head_type == EXP
							&& et->child[q]->child_count == 2
							&& et->child[q]->child[0]->head_type == VARIABLE
							&& sametree(cancel, et->child[q]->child[0])) {
							add_child(new_ptr, 
								copy_tree(et->child[q]->child[1]));
						}
					}


					/* now stick them in the first exponentiation,
					 * and delete the others */
					r = 0;
					for (q = 0; q < et->child_count; ++q) {
						if (et->child[q]->head_type == EXP
							&& et->child[q]->child_count == 2
							&& et->child[q]->child[0]->head_type == VARIABLE
							&& sametree(cancel, et->child[q]->child[0])) {
							if (!r) {
								r = 1;
								et->child[q]->child[1] = new_ptr;
							} else {
								make_tree_number(et->child[q], 1);
							}
						}
					}
					return(1);
				}
			}
		}
	}

	/*
	 *	(A * B^n * C) / B = (A * B ^ (n - 1) * C)
	 */
	if (et->head_type == DIV
		&& et->child_count == 2 
		&& et->child[0]->head_type == MULT) {

		cancel = et->child[1];
		below = et->child[0];

		chk = 0;
		for (q = 0; q < below->child_count; ++q) {
			if (below->child[q]->head_type == EXP
				&& below->child[q]->child_count == 2
				&& sametree(below->child[q]->child[0], cancel)) {
				
				chk = 1;
				new = new_exp_tree(SUB, NULL);
				new_ptr = alloc_exptree(new);
				add_child(new_ptr, copy_tree(below->child[q]->child[1]));
				add_child(new_ptr, new_tree_number(1));
		
				/* transpose */
				memcpy(below->child[q]->child[1], 
					new_ptr, sizeof(exp_tree_t));
			}
		}

		if (chk) {
			et->child_count--;
			return(1);
		}
	}

	/*
	 * A^n * (B + C/A)
	 * = A^n * B + A^(n-1) * C
	 */
	if (et->head_type == MULT
		&& et->child_count == 2
		&& et->child[0]->head_type == EXP
		&& et->child[0]->child_count == 2) {
	
		cancel = et->child[0]->child[0];
		below = et->child[1];
		chk = 0;

		for (q = 0; q < below->child_count; ++q)
			if (below->child[q]->head_type == DIV
				&& below->child[q]->child_count == 2
				&& sametree(below->child[q]->child[1], cancel)) {
					++chk; 
					derp = below->child[q];
				}
	
		if (chk == 1) {
			herp = copy_tree(et->child[0]);

			new2 = new_exp_tree(ADD, NULL);
			new2_ptr = alloc_exptree(new2);

			for (q = 0; q < below->child_count; ++q) {
				if (below->child[q] != derp) {
					new = new_exp_tree(MULT, NULL);
					new_ptr = alloc_exptree(new);
					add_child(new_ptr, copy_tree(herp));
					add_child(new_ptr, copy_tree(below->child[q]));
					add_child(new2_ptr, new_ptr);
				} else {
					num_ptr = copy_tree(herp);
					new = new_exp_tree(SUB, NULL);
					new_ptr = alloc_exptree(new);
					add_child(new_ptr, 
						copy_tree(et->child[0]->child[1]));
					add_child(new_ptr, new_tree_number(1));
					num_ptr->child[1] = new_ptr;

					new = new_exp_tree(MULT, NULL);
					new_ptr = alloc_exptree(new);
					add_child(new_ptr, 
						copy_tree(below->child[q]->child[0]));
					add_child(new_ptr, num_ptr);

					add_child(new2_ptr, new_ptr);
				}
			}

			memcpy(et, new2_ptr, sizeof(exp_tree_t));

			return(1);
		}
	}

	/*
	 * A^n * B * (C + D/A)
	 * = C * A ^ n * B + D * A ^ (n - 1) * B
	 */
	if (et->head_type == MULT) {
		for (i = 0; i < et->child_count; ++i) {
			if (et->child[i]->head_type == EXP
			&& et->child[i]->child_count == 2) {
				cancel = et->child[i];
				
				chk = 0;
				for (q = 0; q < et->child_count; ++q) {
					if (et->child[q]->head_type == ADD) {
						below = et->child[q];
						for (w = 0; w < below->child_count; ++w) {
							if (below->child[w]->head_type == DIV
								&& below->child[w]->child_count == 2
								&& sametree(below->child[w]->child[1],
									cancel->child[0])) {
								++chk;
								chk2 = w;	/* divison subchild */
								chk3 = q;	/* addition child */
							}
						}
					}
				}
			
				if (chk == 1) {
					below = et->child[chk3];
					new = new_exp_tree(ADD, NULL);
					new_ptr = alloc_exptree(new);
					for (q = 0; q < below->child_count; ++q) {
						num = new_exp_tree(MULT, NULL);
						num_ptr = alloc_exptree(num);
						if (q == chk2) {
							derp = copy_tree(below->child[q]);
							derp->child_count--;
							add_child(num_ptr, derp);
						}
						else
							add_child(num_ptr, copy_tree(below->child[q]));

						for (w = 0; w < et->child_count; ++w) {
							if (et->child[w] != below) {
								if (q == chk2 && et->child[w] == cancel) {
									/* bump down power */
									derp = copy_tree(et->child[w]);
									new2 = new_exp_tree(SUB, NULL);
									new2_ptr = alloc_exptree(new2);
									add_child(new2_ptr, copy_tree(cancel->child[1]));
									add_child(new2_ptr, new_tree_number(1));
									derp->child[1] = new2_ptr;

									add_child(num_ptr, derp);
								} else
									add_child(num_ptr, copy_tree(et->child[w]));
							}
						}
						add_child(new_ptr, num_ptr);
					}

					/* transpose and return happily */
					memcpy(et, new_ptr, sizeof(exp_tree_t));
					return 1;
				}
			}
		}
	}

	/* 
	 * 		(A + W + (B * C * X) / U + (C * D * Z) / E) / C
	 * 	=>	(A + W) / C + (B * X) / U + (D * Z) / E
	 *
	 *		((B*C*X)/U + (C*D*Z)/E)/C
	 *	=>	(B * X) / U + (D * Z) / E
	 *
	 * 		(A + B + (C * D * Z) / E) / C
	 * 	=>	(A + B) / C + (D * Z) / E
	 *
	 */
	if (et->head_type == DIV
		&& et->child_count == 2
		&& et->child[0]->head_type == ADD) {

		chk = 0;

		below = et->child[0];
		cancel = et->child[1];

		for (q = 0; q < below->child_count; ++q)
			if (below->child[q]->head_type == DIV
				&& below->child[q]->child_count == 2
				&& below->child[q]->child[0]->head_type == MULT) {
				for (w = 0; w < below->child[q]->child[0]->child_count; ++w) {
					if (sametree(cancel, 
						below->child[q]->child[0]->child[w]))
					{
						++chk;
						break;
					}
				}
			}

		if (chk >= 1) {
			/* "A + W" part */
			new = new_exp_tree(ADD, NULL);
			new_ptr = alloc_exptree(new);

			chk2 = 0;
			for (q = 0; q < below->child_count; ++q) {
				chk = 0;

				if (below->child[q]->head_type == DIV
					&& below->child[q]->child_count == 2
					&& below->child[q]->child[0]->head_type == MULT) {
					for (w = 0; w < below->child[q]->child[0]->child_count; ++w)
						if (sametree(cancel, 
							below->child[q]->child[0]->child[w])) {
							chk = 1;
							break;
						}
				}

				if (!chk) {
					++chk2;
					add_child(new_ptr, copy_tree(below->child[q]));
				}
			}

			/* divide by "C" to make "(A + W) / C" */
			if (chk2) {
				new2 = new_exp_tree(DIV, NULL);
				new2_ptr = alloc_exptree(new2);
				add_child(new2_ptr, new_ptr);
				add_child(new2_ptr, copy_tree(et->child[1]));
			}

			/* =============================== */

			/* make [(A+W)/C + ...] */
			num = new_exp_tree(ADD, NULL);
			num_ptr = alloc_exptree(num);
			if (chk2)
				add_child(num_ptr, new2_ptr);

			/* add cancelled-out terms to the sum */
			below = et->child[0];
			cancel = et->child[1];

			for (q = 0; q < below->child_count; ++q)
				if (below->child[q]->head_type == DIV
					&& below->child[q]->child_count == 2
					&& below->child[q]->child[0]->head_type == MULT) {
						for (w = 0; w < below->child[q]->child[0]->child_count; ++w) {
							if (sametree(cancel, 
								below->child[q]->child[0]->child[w]))
							{
								derp = below->child[q];

								for (e = 0; e < derp->child[0]->child_count; ++e)
									if (sametree(derp->child[0]->child[e], cancel))
										make_tree_number(derp->child[0]->child[e], 1);

								add_child(num_ptr, copy_tree(derp));
							}
						}
				}

			memcpy(et, num_ptr, sizeof(exp_tree_t));
			return(1);			
		}
	}

	/* 
	 *    (A * B^n) / (B * C * D)	
	 * => (A * B ^ (n - 1)) / (C * D)
	 */
	if (et->head_type == DIV
		&& et->child_count == 2
		&& et->child[0]->head_type == MULT
		&& et->child[1]->head_type == MULT) {
		below = et->child[0];
		derp = et->child[1];
		for (q = 0; q < below->child_count; ++q) {
			if (below->child[q]->head_type == EXP
				&& below->child[q]->child_count == 2) {
				cancel = below->child[q];
				chk = 0;
				for (w = 0; w < derp->child_count; ++w)
					if (sametree(derp->child[w], cancel->child[0])) {
						chk2 = w;
						if (++chk > 1)
							break;
					}

				if (chk == 1) {
					make_tree_number(derp->child[chk2], 1);
					num = new_exp_tree(SUB, NULL);
					num_ptr = alloc_exptree(num);
					add_child(num_ptr, copy_tree(cancel->child[1]));
					add_child(num_ptr, new_tree_number(1));
					cancel->child[1] = num_ptr;
					return 1;
				}
			}
		}
	}

	/*
	 * --A = A
	 */
	if (et->head_type == NEGATIVE
		&& et->child_count == 1
		&& et->child[0]->head_type == NEGATIVE
		&& et->child[0]->child_count == 1) {

		memcpy(et, et->child[0]->child[0], sizeof(exp_tree_t));
		return 1;
	}

	/*
	 * A * U/A  * FOO * BAR * XYZ
	 * => U * FOO * BAR * XYZ
	 *
	 * When applied repeatedly, it can do e.g.
	 * 		A * U/A * BAR/A * A * A * XYZ
	 * 		=> U * BAR * A * XYZ
	 */
	if (et->head_type == MULT
		&& et->child_count >= 2) {

		for (q = 0; q < et->child_count; ++q) {
			cancel = et->child[q];

			/* A must be unique */
			chk = 0;
			for (w = 0; w < et->child_count; ++w)
				if (sametree(et->child[w], cancel))
					if (++chk > 1)
						break;
			if (chk != 1)
				break;

			/* and there must be one instance of U / A */
			chk = 0;
			for (w = 0; w < et->child_count; ++w) {
				if (et->child[w]->head_type == DIV
					&& et->child[w]->child_count == 2
					&& sametree(et->child[w]->child[1], cancel)) {
					if (++chk > 1)
						break;
					else
						chk2 = w;
				}				
			}

			/* all conditions fulfilled, do it */
			if (chk == 1) {
				make_tree_number(et->child[q], 1);
				et->child[chk2]->child_count--;
				return 1;
			}
		}
	}

	/*
	 * A * (B * C * ... * F) / G 
	 * => (A * B * C .. * F) / G
	 */
	if (et->head_type == MULT
		&& et->child_count == 2
		&& et->child[1]->head_type == DIV
		&& et->child[1]->child_count == 2
		&& et->child[1]->child[0]->head_type == MULT) {

		add_child(et->child[1]->child[0], copy_tree(et->child[0]));
		make_tree_number(et->child[0], 1);
		return 1;
	}

	/*
	 * 	   	donald / (u * x ^ n) * x * foo
	 * =>  	donald / (u * x ^ (n - 1)) * foo
	 */
	if (et->head_type == MULT) {
		for (q = 0; q < et->child_count; ++q) {
			cancel = et->child[q];
			for (w = 0; w < et->child_count; ++w) {
				if (et->child[w]->head_type == DIV
					&& et->child[w]->child_count == 2
					&& et->child[w]->child[1]->head_type == MULT) {
					below = et->child[w]->child[1];
					chk = 0;
					for (e = 0; e < below->child_count; ++e) {
						if(below->child[e]->head_type == EXP
							&& below->child[e]->child_count == 2
							&& sametree(below->child[e]->child[0], cancel)) {
							chk = 1;
							break;
						}
					}
					if (chk) {
						make_tree_number(cancel, 1);
						num = new_exp_tree(SUB, NULL);
						num_ptr = alloc_exptree(num);
						add_child(num_ptr, copy_tree(below->child[e]->child[1]));
						add_child(num_ptr, new_tree_number(1));
						below->child[e]->child[1] = num_ptr;
						return 1;
					}
				}
			}
		}
	}

	/* 
	 * N-term sum rule
	 * D(u, a + b + c + ...) = D(u, a) + D(u, b) + D(u, c) + ...
	 */
	if (et->head_type == FUNC
		&& *(et->tok->start) == 'D'
		&& et->tok->len == 1
		&& et->child_count == 2
		&& et->child[1]->head_type == ADD
		&& et->child[1]->child_count > 1) {

		new = new_exp_tree(ADD, NULL);
		new_ptr = alloc_exptree(new);

		for (q = 0; q < et->child[1]->child_count; ++q) {
			num_ptr = copy_tree(et);
			num_ptr->child[1] = copy_tree(et->child[1]->child[q]);
			add_child(new_ptr, num_ptr);
		}

		memcpy(et, new_ptr, sizeof(exp_tree_t));
		return(1);
	}

	/* anything times zero is 0 */
	if (et->head_type == MULT
		&& et->child_count > 1) {
		for (q = 0; q < et->child_count; ++q) {
			if (et->child[q]->head_type == NUMBER
				&& tok2int(et->child[q]->tok) == 0) {
				memcpy(et, new_tree_number(0), sizeof(exp_tree_t));
				return(1);
			}
		}
	}

	return did;
}

#ifdef DEBUG
	#define return(X) return(X)
#endif

/*
 * Parser (tokens -> tree)
 *
 * (this code was copied and adapted from 
 * my other project, the wannabe compiler)
 *
 * BUGS: seems to get into infinite recursions
 * or segfaults on certain occurences of 
 * invalid syntax
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

token_t *tokens;
int indx;
int tok_count;

exp_tree_t block();
exp_tree_t expr();
exp_tree_t pow_expr();
exp_tree_t sum_expr();
exp_tree_t mul_expr();
exp_tree_t unary_expr();
exp_tree_t signed_expr();
void printout(exp_tree_t et);
void fail(char* mesg);

/* give the current token */
token_t peek()
{
	token_t t;
	if (indx >= tok_count) {
		t = tokens[tok_count - 1];
	}
	else {
		t = tokens[indx];
	}
	return t;
}

/* generate a pretty error when parsing fails.
 * this is a hack-wrapper around the routine
 * in diagnostics.c that deals with certain
 * special cases special to the parser. */
void parse_fail(char *message)
{
	token_t tok;
	int line, chr;

	if (tokens[indx].from_line
		!= tokens[indx - 1].from_line) {
		/* end of the line */
		line = tokens[indx - 1].from_line;
		chr = strstr(code_lines[line], "\n")
			- code_lines[line] + 1;
	} else {
		line = tokens[indx].from_line;
		chr = tokens[indx].from_char;
	}

	compiler_fail(message, NULL, line, chr);
}

/*
 * Fail if the next token isn't of the desired type.
 * Otherwise, return it and advance.
 */
token_t need(char type)
{
	char buf[1024];
	if (tokens[indx++].type != type) {
		fflush(stdout);
		--indx;
		sprintf(buf, "%d expected", type);
		parse_fail(buf);
	} else {
		token_t t = tokens[indx - 1];
		return t;
	}
}

/* main parsing loop */
exp_tree_t parse(token_t *t)
{
	exp_tree_t program;
	exp_tree_t *subtree;
	program = new_exp_tree(BLOCK, NULL);
	int i;

	null_tree.head_type = NULL_TREE;

	/* count the tokens */
	for (i = 0; t[i].start; ++i)
		;
	tok_count = i;

	/* the loop */
	tokens = t;
	indx = 0;
	while (tokens[indx].start) {
		subtree = alloc_exptree(block());
		/* parse fails at EOF */
		if (!valid_tree(*subtree))
			break;
		add_child(&program, subtree);
	}

	return program;
}

/* 
 * Now the real fun starts. Each routine is
 * named after the part of the grammar that
 * it deals with. When it doesn't find anything
 * that it's been trained to find, a routine
 * gives back "null_tree".
 */


/*
 *	variables (any text symbol)
 * 	quoted symbols : ' text
 */
exp_tree_t lval()
{
	token_t tok = peek();
	token_t tok2;
	exp_tree_t tree, subtree;
	int cc = 0;

	if (tok.type == TOK_NAN) {
		++indx;	/* eat @ */
		tok = peek();
		++indx;
		return new_exp_tree(MATCH_NAN, &tok);
	} else if (tok.type == TOK_MVAR) {
		++indx;	/* eat ? */
		tok = peek();
		++indx;	/* eat ident */
		return new_exp_tree(MATCH_VAR, &tok);
	}
	else if (tok.type == TOK_MNUM) {
		++indx;	/* eat # */
		tok = peek();
		++indx;	/* eat int */
		return new_exp_tree(MATCH_NUM, &tok);
	}
	else if (tok.type == TOK_QUOT) {
		++indx;	/* eat quote */
		tok = peek();
		++indx;	/* eat ident */
		return new_exp_tree(SYMBOL, &tok);
	}
	else if (tok.type == TOK_IDENT) {
			++indx;	/* eat the identifier */
			if (peek().type == TOK_NOTSYMBOL) {
				++indx;
				tok2 = peek();
				++indx;
				if (tok2.type != TOK_IDENT)
					parse_fail("plain symbol expected");
				tree = new_exp_tree(MATCH_IND_SYM, NULL);
				subtree = new_exp_tree(VARIABLE, &tok);
				add_child(&tree, alloc_exptree(subtree));
				subtree = new_exp_tree(VARIABLE, &tok2);
				add_child(&tree, alloc_exptree(subtree));
				return tree;
			} else if (peek().type == TOK_PIPE) {
				++indx;
				tok2 = peek();
				++indx;
				if (tok2.type != TOK_IDENT)
					parse_fail("plain symbol expected");
				tree = new_exp_tree(MATCH_IND, NULL);
				subtree = new_exp_tree(VARIABLE, &tok);
				add_child(&tree, alloc_exptree(subtree));
				subtree = new_exp_tree(VARIABLE, &tok2);
				add_child(&tree, alloc_exptree(subtree));
				return tree;
			} else 	if (peek().type == TOK_COLON
				|| peek().type == TOK_CC) {
				cc = peek().type == TOK_CC;
				++indx;
				tok2 = peek();
				++indx;
				if (tok2.type != TOK_IDENT)
					parse_fail("plain symbol expected");
				tree = new_exp_tree(cc ? MATCH_CONTX
					: MATCH_CONT, NULL);
				subtree = new_exp_tree(VARIABLE, &tok);
				add_child(&tree, alloc_exptree(subtree));
				subtree = new_exp_tree(VARIABLE, &tok2);
				add_child(&tree, alloc_exptree(subtree));
				return tree;
			} else
				return new_exp_tree(VARIABLE, &tok);
	}
	return null_tree;
}

exp_tree_t block()
{
	return expr();
}

/*
	expr :=  sum_expr ['=' sum_expr]
*/
exp_tree_t expr()
{
	exp_tree_t tree, subtree, subtree2;
	exp_tree_t subtree3;
	exp_tree_t lv;
	token_t oper;
	token_t tok;
	int save = indx;

	/* sum_expr ['=' sum_expr] */
	if (valid_tree(subtree = sum_expr())) {
		if (peek().type != TOK_ASGN) {
			return subtree;
		}
		if (peek().type == TOK_ASGN) {
			tree = new_exp_tree(EQL, NULL);
			++indx;	/* eat comp-op */
			add_child(&tree, alloc_exptree(subtree));
			if (!valid_tree(subtree2 = sum_expr()))
				parse_fail("expression expected");
			add_child(&tree, alloc_exptree(subtree2));
		}
		return tree;
	}

	return null_tree;
}

/* sum_expr := mul_expr { add-op mul_expr } */
exp_tree_t sum_expr()
{
	exp_tree_t child, tree, new;
	exp_tree_t *child_ptr, *tree_ptr, *new_ptr;
	exp_tree_t *root;
	int prev;
	token_t oper;

	if (!valid_tree(child = mul_expr()))
		return null_tree;
	
	if (!is_add_op((oper = peek()).type))
		return child;
	
	child_ptr = alloc_exptree(child);
	
	switch (oper.type) {
		case TOK_PLUS:
			tree = new_exp_tree(ADD, NULL);
		break;
		case TOK_MINUS:
			tree = new_exp_tree(SUB, NULL);
		break;
	}
	prev = oper.type;
	++indx;	/* eat add-op */
	tree_ptr = alloc_exptree(tree);
	add_child(tree_ptr, child_ptr);

	while (1) {
		if (!valid_tree(child = mul_expr()))
			parse_fail("expression expected");

		/* add term as child */
		add_child(tree_ptr, alloc_exptree(child));

		/* bail out early if no more operators */
		if (!is_add_op((oper = peek()).type))
			return *tree_ptr;

		switch (oper.type) {
			case TOK_PLUS:
				new = new_exp_tree(ADD, NULL);
			break;
			case TOK_MINUS:
				new = new_exp_tree(SUB, NULL);
			break;
		}
		++indx;	/* eat add-op */

		new_ptr = alloc_exptree(new);
		add_child(new_ptr, tree_ptr);
		tree_ptr = new_ptr;
	}

	return *tree_ptr;
}

/* mul_expr := pow_expr { mul-op pow_expr } */
exp_tree_t mul_expr()
{
	/* this routine is mostly a repeat of sum_expr() */
	exp_tree_t child, tree, new;
	exp_tree_t *child_ptr, *tree_ptr, *new_ptr;
	exp_tree_t *root;
	int prev;
	token_t oper;

	if (!valid_tree(child = signed_expr()))
		return null_tree;
	
	if (!is_mul_op((oper = peek()).type))
		return child;
	
	child_ptr = alloc_exptree(child);
	
	switch (oper.type) {
		case TOK_DIV:
			tree = new_exp_tree(DIV, NULL);
		break;
		case TOK_MUL:
			tree = new_exp_tree(MULT, NULL);
		break;
	}
	prev = oper.type;
	++indx;	/* eat add-op */
	tree_ptr = alloc_exptree(tree);
	add_child(tree_ptr, child_ptr);

	while (1) {
		if (!valid_tree(child = signed_expr()))
			parse_fail("expression expected");

		/* add term as child */
		add_child(tree_ptr, alloc_exptree(child));

		/* bail out early if no more operators */
		if (!is_mul_op((oper = peek()).type))
			return *tree_ptr;

		switch (oper.type) {
			case TOK_DIV:
				new = new_exp_tree(DIV, NULL);
			break;
			case TOK_MUL:
				new = new_exp_tree(MULT, NULL);
			break;
		}
		++indx;	/* eat add-op */

		new_ptr = alloc_exptree(new);
		add_child(new_ptr, tree_ptr);
		tree_ptr = new_ptr;
	}

	return *tree_ptr;
}

/*
 	signed_expr := [-] pow_expr
*/
exp_tree_t signed_expr()
{
	token_t tok = peek();
	exp_tree_t tree, subtree;

	if (tok.type == TOK_MINUS) {
		++indx;
		tree = new_exp_tree(NEGATIVE, NULL);
		if (valid_tree(subtree = pow_expr()))
			add_child(&tree, alloc_exptree(subtree));
		else
			parse_fail("expression expected after sign");
		return tree;
	} else
		return pow_expr();
}

/*
	pow_expr :=  unary_expr ['^' expr]
*/
exp_tree_t pow_expr()
{
	exp_tree_t tree, subtree, subtree2;
	exp_tree_t subtree3;
	exp_tree_t lv;
	token_t oper;
	token_t tok;
	int save = indx;

	if (valid_tree(subtree = unary_expr())) {
		if (peek().type != TOK_EXP) {
			return subtree;
		}
		else {
			tree = new_exp_tree(EXP, NULL);
			++indx;	/* eat comp-op */
			add_child(&tree, alloc_exptree(subtree));
			if (!valid_tree(subtree2 = pow_expr()))
				parse_fail("expression expected");
			add_child(&tree, alloc_exptree(subtree2));
		}
		return tree;
	}

	return null_tree;
}


/*
	unary_expr :=  lvalue | integer
			|  '(' expr ')' 
			| ident '(' expr1, expr2, exprN ')'
			| - expr
*/
exp_tree_t unary_expr()
{
	exp_tree_t tree = null_tree, subtree = null_tree;
	exp_tree_t subtree2 = null_tree, subtree3;
	token_t tok;
	int matchfunc = 0;

	/* $ident (expr1, ..., exprN) */
	if (peek().type == TOK_DOLLAR) {
		++indx;
		matchfunc = 1;
	}

	/* ident ( expr1, expr2, exprN ) */
	if (peek().type == TOK_IDENT) {
		tok = peek();
		++indx;
		if (peek().type == TOK_LPAREN) {
			++indx;	/* eat ( */
			subtree = new_exp_tree(matchfunc ? MATCH_FUNC :
				FUNC, &tok);
			while (peek().type != TOK_RPAREN) {
				subtree2 = expr();
				if (!valid_tree(subtree2))
					parse_fail("expression expected");
				add_child(&subtree, alloc_exptree(subtree2));
				if (peek().type != TOK_RPAREN)
					need(TOK_COMMA);
			}
			++indx;
			/* if there's already a negative sign,
			 * use that as a parent tree */
			if (valid_tree(tree))
				add_child(&tree, alloc_exptree(subtree));
			else
				tree = subtree;
			return tree;
		} else
			--indx;
	}

	/* parenthesized expression */
	if(peek().type == TOK_LPAREN) {
		++indx;	/* eat ( */
		if (!valid_tree(tree = expr()))
			parse_fail("expression expected");
		need(TOK_RPAREN);
		
		return tree;
	}

	tok = peek();
	
	if (tok.type == TOK_INTEGER) {
		tree = new_exp_tree(NUMBER, &tok);
		++indx;	/* eat number */

		return tree;
	}

	if (valid_tree(tree = lval())) {
		return tree;
	}

	if (tok.type == TOK_MINUS) {
		++indx;
		tree = new_exp_tree(NEGATIVE, NULL);
		if (valid_tree(subtree = pow_expr()))
			add_child(&tree, alloc_exptree(subtree));
		else
			parse_fail("expression expected after sign");
		return tree;
	}

	return null_tree;
}


//#include "gc.h"

/*
 * This is pretty boring code which reads
 * all the lines in all the files in the
 * directory "rules", parses them and
 * adds the resulting trees into the
 * rule-list.
 */

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include "tokenizer.h"
//#include "tokens.h"
//#include "tree.h"
//#include "parser.h"
//#include "dict.h"
//#include "rulefiles.h"
//#include "optimize.h"

void fail(char*);

int readrules(exp_tree_t** rules, char *dir)
{
	void* f  = NULL;
	void* index = NULL;
	char lin[1024];
	char path[1024];
	char filename[1024];
	int rc = 0;

	printf("Reading rules from '%s'...", dir);
	fflush(stdout);

	/*
	 * Originally, this code relied on the OS
	 * to list the files in the rulefile directories.
	 * This worked, however there was a problem:
	 * it seems depending on the OS the files were shown in
	 * varying orders, leading to differing results.
	 * So now (2014-01-26) there are "_index" 
	 * files that explicitly hardcode the loading order.
	 *
	 * For testing purposes, the following should hold:
	 *		]=> diff(x^x^x, x)
	 * 		x ^ (x ^ x + x) * log(x) * (1 + log(x)) + x ^ ((x ^ x + x) - 1)
	 */

	sprintf(path, "%s/_index", dir);
	if ((index = fopen(path, "r"))) {
		for (;;) {
			if (feof(index))
				break;
			fscanf(index, "%s", filename);

			if (!*filename)
				break;

			sprintf(path, "%s/%s", dir, filename);

			if ((f = fopen(path, "r"))) {
				while (1) {
					if (!fgets(lin, 1024, f))
						break;
					/* skips comments and blanks ... */
					if (!lin[1] || !*lin || *lin == '%')
						continue;
					else {
						lin[strlen(lin) - 1] = 0;
						rule(lin, rules, &rc);
					}
				}
				fclose(f);
			} else
				printf("\nCouldn't open rule-file '%s'\n", path);
		}
		printf("Done.\n");
		fclose(index);
	} else
		printf("\nCouldn't open index file for rules directory '%s'\n", dir);

	return rc;	/* rule count */
}

void rule(char *r, exp_tree_t **rules, int* rc)
{
	token_t* tokens;
	exp_tree_t tree;

	void fail(char*);

	/* lexing */
	tokens = tokenize(r);


	/* parsing */
	tree = *((parse(tokens)).child[0]);
	/* optimize(&tree); */

	/* some preventive checking */
	if (tree.head_type != EQL) {
		printf("\n");
		fail("you asked me to store, as a rule, a non-rule expression :(");
		
		printf("\n");
	}

	rules[(*rc)++] = alloc_exptree(tree);

}


//#include "gc.h"

/*
 * Substitution routine
 */

//#include "dict.h"
//#include "tree.h"
//#include "subst.h"
//#include <string.h>
//#include <stdio.h>


/* Find index of symbol in dictionary array */
int findkey(char *buf, dict_t* d)
{
	int i;
	for (i = 0; i < d->count; ++i)
		if (!strcmp(d->symbol[i], buf))
			return i;
	return -1;
}

void subst(exp_tree_t *tree, dict_t *d, exp_tree_t **father_ptr)
{
	int i, key;

	/* Function name substitution */
	if (tree->head_type == FUNC) {
		/* Get metasymbol */
		strncpy(buf, tree->tok->start, tree->tok->len);
		buf[tree->tok->len] = 0;

		if((key = findkey(buf, d)) != -1) {
			if (!father_ptr)
				fail("null pointer substitution....");
			/* 
			 * Don't substitute trees, but instead 
			 * change the tree head label
			 */
			(*father_ptr)->tok = d->match[key]->tok;
			/* Then go substitute the children */
			goto match_children;
		}
	}

	/* General case */
	if (tree->head_type == VARIABLE) {
		strncpy(buf, tree->tok->start, tree->tok->len);
		buf[tree->tok->len] = 0;
		if((key = findkey(buf, d)) != -1) {
			if (!father_ptr)
				fail("null pointer substitution....");
			*father_ptr = copy_tree(d->match[key]);
		}
	} else {
match_children:
		/* Children recursion */
		for (i = 0; i < tree->child_count; ++i) {
			if(tree->child[i]->head_type == VARIABLE
			|| tree->child[i]->head_type == FUNC
			|| tree->child[i]->child_count)
				subst(tree->child[i], d, &(tree->child[i]));
		}
	}
}
//#include "gc.h"

/*
 * Tokenization code based on wannabe-regex.c,
 * which I wrote some months ago
 */

/* TODO: cleanup some of the more mysterious
 *       parts of the code...
 * FIXME: the automatons never get free()'d !
 */

//#include "tokenizer.h"
//#include "tokens.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>



typedef struct match_struct {
	/* accept number, if any */
	int token;
	
	/* pointer to last character matched */
	char* pos;
} match_t;

typedef struct trie_struct {
	/* character edges */
	struct trie_struct* map[256];

	/* epsilon edges */
	struct trie_struct** link;
	int links;
	int link_alloc;

	/* accept state, if any */
	int valid_token;
} trie;

/* ============================================ */

/* Trie routines */

void add_link(trie* from, trie* to)
{
	if (++from->links > from->link_alloc)
		from->link = realloc(from->link, 
			(from->link_alloc += 5) * sizeof(trie *));

	if (!from->link)
		fail("link array allocation");

	from->link[from->links - 1] = to;
}

trie* new_trie()
{
	trie* t = cgc_malloc(sizeof(trie));
	int i;
	if (!t)
		fail("trie allocation");
	t->link = cgc_malloc(10 * sizeof(trie*));
	if (!t->link)
		fail("trie links");
	t->links = 0;
	t->link_alloc = 10;
	for (i = 0; i < 10; i++)
		t->link[i] = NULL;
	t->valid_token = 0;
	for(i = 0; i < 256; i++)
		t->map[i] = NULL;
	return t;
}

/* ============================================ */

/* Pattern compiler routine */

void add_token(trie* t, char* tok, int key)
{
	int i, j;
	int c;
	trie *hist[1024];
	trie *next;
	int len = strlen(tok);
	char* new_tok;
	int n;

	new_tok = cgc_malloc(len + 32);
	if (!new_tok)
		fail("buffer allocation");
	strcpy(new_tok, tok);

	for (i = 0, n = 0; c = new_tok[i], i < len && t; ++i)
	{
		hist[n] = t;

		switch (c) {
			case 'A':	/* special: set of all letters */
				next = new_trie();
				for(j = 'a'; j <= 'z'; j++)
					t->map[j] = next;
				for(j = 'A'; j <= 'Z'; j++)
					t->map[j] = next;
				t->map['_'] = next;
				t = next;
				++n;
				break;
			case 'B':	/* special: set of all letters 
					   + all digits */
				next = new_trie();
				for(j = 'a'; j <= 'z'; j++)
					t->map[j] = next;
				for(j = 'A'; j <= 'Z'; j++)
					t->map[j] = next;
				for(j = '0'; j <= '9'; j++)
					t->map[j] = next;
				t->map['_'] = next;
				t = next;
				++n;
				break;
			case 'D':	/* special: set of all digits */
				next = new_trie();
				for(j = '0'; j <= '9'; j++)
					t->map[j] = next;
				t = next;
				++n;
				break;
			case 'W':	/* special: whitespace */
				next = new_trie();
				t->map[' '] = next;
				t->map['\t'] = next;
				t = next;
				++n;
				break;
			case '?':	/* optional character */
				sanity_requires(n > 0);

				/* 
				 * The previous node is allowed to
				 * go straight to this node, via
				 * an epsilon link.
				 */
				add_link(hist[n - 1], t);
				break;
			case '+':	/* character can repeat
					   any number of times */
				sanity_requires(n > 0);

				/* 
				 * Make a "buffer" node to keep things
				 * cleanly separated and avoid
				 * surprises when matching e.g.
				 * 'a+b+'. Epsilon-link to it.
				 */
				add_link(t, hist[++n] = new_trie());
				t = hist[n];

				/* 
				 * Next, we epsilon-link the buffer node
				 * all the way back to the node '+' compilation
				 * started with, to allow repetition.
				 */
				add_link(t, hist[n - 2]);

				/* 
				 * And finally we make a clean
				 * new node for further work,
				 * and epsilon-link to it.
				 */
				add_link(t, hist[++n] = new_trie());
				t = hist[n];
				break;
			case '*':	/* optional character
					   with repetition allowed */
				sanity_requires(n > 0);

				/* same as +, except that the
 				 * second part makes a mutual link */

				add_link(t, hist[++n] = new_trie());
				t = hist[n];
				
				/* mutual link */
				add_link(t, hist[n - 2]);
				add_link(hist[n - 2], t);

				add_link(t, hist[++n] = new_trie());
				t = hist[n];
				break;
			case '\\':	/* escape to normal text */
				if (++i == len)
					fail("backslash expected char");
				c = new_tok[i];
			default:	/* normal text */
				if (!t->map[c])
					t->map[c] = new_trie();
				t = t->map[c];
				++n;
		}
	}

	/* 
	 * We have reached the accept-state node;
	 * mark it as such.
	 */
	t->valid_token = key;
}


/*
 * NFA trie evaluator (uses backtracking) 
 * Has implementation-time hacks, parameters and all
 * (aka bloat)
 */

match_t match_routine(trie* automaton, char* full_text, char* where)
{
	match_t match_algo(trie* t, char* full_tok, char* tok,
		int frk, trie* led, int ledi);

	match_t m = match_algo(automaton, full_text, where, 0, NULL, 0);
	return m;
}

/* trie* t		:	matching automaton / current node
 * char* full_tok	:	full text
 * char* tok		: 	where we are now
 * int frk		: 	internal. initially, give 0
 * trie* led		:	internal. start with NULL
 * int ledi		:	internal. start with 0
 */	
match_t match_algo(trie* t, char* full_tok, char* tok, int frk, 
	trie* led, int ledi)
{
	int i = 0;
	int j;
	char c;

	/* greedy choice registers */
	int record;		/* longest match */
	match_t m;		/* match register */
	match_t ml[10];		/* match list */
	int mc = 0;		/* match count */
	match_t* choice;	/* final choice */	

	int len = strlen(tok) + 1;

	int last_inc;

	i = 0;
	dl2:
	{
		last_inc = 0;
		c = tok[i];

		/* check if multiple epsilon edges exist
		 * and if we are not already in a fork */ 
		if (t->links && !frk)
		{
			/* fork for each possibility
			 * and collect results in an array */

			/* character edge: frk = 1 */
			if (t->map[c])
				if ((m = match_algo(t, full_tok, tok + i, 1,
					led, ledi)).token)
					ml[mc++] = m;

			/* epsilon edge(s): frk = 2 + link number */
			for (j = 0; j < t->links; j++)
				if ((m = match_algo(t, full_tok, tok + i, 
					j + 2, led, ledi)).token)
					ml[mc++] = m;

			/* if there are multiple epsilon matches,
			 * be "greedy", i.e. choose longest match. */
			if (mc > 1) {
				record = 0;
				choice = NULL;
				for (j = 0; j < mc; j++)
					if (ml[j].pos - full_tok >= record) {
						choice = &ml[j];
						record = ml[j].pos - full_tok;
					}
				if (!choice) fail("greedy choice");
				return *choice;
			}
			else if (mc == 1) /* only one match */
				return *ml;
			else	/* no match, return blank as-is */
				return m;
		}

		/* if we are not in an epsilon fork
		 * and the character edge exists,
		 * follow it. */
		if (frk <= 1 && t->map[c]) {
			t = t->map[c];

			last_inc = 1;

			/* break loop if in matching state */
			if (t->valid_token) {
				goto break_dl2;
			}

			++i;
			goto valid;
		}
		else if (frk > 1) {
			/* prevents infinite loops: if the
			 * link chosen in this fork brings us back
			 * to the last link followed and the character
			 * index has not increased since we followed 
			 * this last link, do not follow it */
			if (!(led && t->link[frk - 2] == led
			    && (int)(&tok[i] - full_tok) == ledi)) {

				/* t->links == 1 is special (???) */
				if (t->links > 1) {
					led = t; /* Last Epsilon departure
						    node */
					ledi = (int)(&tok[i] - full_tok);
				}

				t = t->link[frk - 2];
					
				goto valid;
			}
		}

		goto break_dl2;

		/* clear fork path number after the first
		 * iteration of the loop, it is not relevant
		 * afterwards */
valid:		if (frk)
			frk = 0;

	}
	if (i < len && t)
		goto dl2;
	break_dl2:

	/* set up the returned structure */
	if (t && t->valid_token) {
		/* match */
		m.token = t->valid_token;
		m.pos = tok + i + last_inc;
	} else {
		/* no match */
		m.token = 0;
	}

	return m;
}

trie *t[100];
int tc = 0;

token_t* tokenize(char *in_buf)
{
	match_t m;
	match_t c;
	char buf2[1024];
	int max;
	int i;
	int line = 1;
	char *line_start;
	token_t *toks = cgc_malloc(64 * sizeof(token_t));
	int tok_alloc = 64;
	int tok_count = 0;
	int in_comment = 0;
	char *buf = cgc_malloc(strlen(in_buf) + 1);
	char *p;

	strcpy(buf, in_buf);
	buf[strlen(in_buf)] = 0;

	*code_lines = buf;
	line_start = buf;


	for(p = buf; *p; ) {
		max = -1;

		/* 
		 * Choose the matching pattern
		 * that gives the longest match.
		 * Shit is "meta-greedy".
		 */
		for (i = 0; i < tc; i++) {
			m = match_routine(t[i], buf, p);
			if (m.token) {	/* match has succeeded */
				if (m.pos - p > max) {
					c = m;
					max = m.pos - p;
				}
			}
		}

		if (max == -1) {
			/* matching from this offset failed */
			if (in_comment) {
				/* okay to have non-tokens in comments */
				++p;
				continue;
			}
			fail("tokenization failed");
			
		} else {


			/* comments */
			if (c.token == C_CMNT_OPEN) {
				if (in_comment == 2)
					fail("don't nest comments, please");
				in_comment = 2;
			} else if (c.token == C_CMNT_CLOSE) {
				if (in_comment == 2) {
					in_comment = 0;
					goto advance;
				} else {
					fail("dafuq is a */ doing there ?");
				}
			} else if (c.token == CPP_CMNT) {
				if (in_comment == 2)
					fail("don't mix and nest comments, please");
				in_comment = 1;		
			}
			if (in_comment == 1 && c.token == TOK_NEWLINE) {
				in_comment = 0;
				goto advance;
			}

			/* add token to the tokens array */
			if (c.token != TOK_WHITESPACE
			 && c.token != TOK_NEWLINE
			 && !in_comment) {
				if (++tok_count > tok_alloc) {
					tok_alloc = tok_count + 64;
					toks = realloc(toks,
						tok_alloc * sizeof(token_t));
					if (!toks)
						fail("resize tokens array");
				}
				toks[tok_count - 1].type = c.token;
				toks[tok_count - 1].start = p;
				toks[tok_count - 1].len = max;
				toks[tok_count - 1].from_line = line;
				toks[tok_count - 1].from_char = p - 
					line_start + 1;
			}

advance:
			/* move forward in the string */
			p += max;
			if (max == 0)
				++p;

			/* line accounting (useful for
		 	 * pretty parse-fail diagnostics) */
			if (c.token == TOK_NEWLINE) {
				code_lines[line] = line_start;
				++line;
				line_start = p;
			}
		}
	}

	/* the final token is a bit special */
	toks[tok_count].start = NULL;
	toks[tok_count].len = 0;
	toks[tok_count].from_line = line;
	toks[tok_count].from_char = p - line_start;
	code_lines[line] = line_start;

	return toks;
}

void setup_tokenizer()
{
	int i = 0;

	/* set up the automatons that
	 * match the various token types */

	for (i = 0; i < 100; i++)
		t[i] = new_trie();

	/* D: any digit */
	add_token(t[tc++], "D+", TOK_INTEGER);

	/* W: any whitespace character */
	add_token(t[tc++], "W+", TOK_WHITESPACE);

	/* A: letter; B: letter or digit */
	add_token(t[tc++], "AB*", TOK_IDENT);

	/* operators */
	add_token(t[tc++], "\\+", TOK_PLUS);
	add_token(t[tc++], "-", TOK_MINUS);
	add_token(t[tc++], "/", TOK_DIV);
	add_token(t[tc++], "\\*", TOK_MUL);
	add_token(t[tc++], "=", TOK_ASGN);

	/* special characters */

	add_token(t[tc++], "(", TOK_LPAREN);
	add_token(t[tc++], ")", TOK_RPAREN);
	add_token(t[tc++], ";", TOK_SEMICOLON);
	add_token(t[tc++], ",", TOK_COMMA);
	add_token(t[tc++], "\n", TOK_NEWLINE);
	add_token(t[tc++], ":", TOK_COLON);
	add_token(t[tc++], "[", TOK_LBRACK);
	add_token(t[tc++], "]", TOK_RBRACK);

	add_token(t[tc++], "'", TOK_QUOT);
	add_token(t[tc++], "@", TOK_NAN);

	add_token(t[tc++], "#", TOK_MNUM);
	add_token(t[tc++], "\\?", TOK_MVAR);
	add_token(t[tc++], "|", TOK_PIPE);
	add_token(t[tc++], "\\$", TOK_DOLLAR);

	add_token(t[tc++], "\\^", TOK_EXP);

	add_token(t[tc++], "::", TOK_CC);

	add_token(t[tc++], "|'", TOK_NOTSYMBOL);

	/* comments */
	add_token(t[tc++], "/\\*", C_CMNT_OPEN);
	add_token(t[tc++], "\\*/", C_CMNT_CLOSE);
	add_token(t[tc++], "//", CPP_CMNT);
}

//#include "gc.h"

/* Token helper routines */

//#include "tokens.h"
//#include <string.h>
//#include <stdio.h>

void fail(char* mesg);

/* Print out a token's raw string */
void tok_display(void *f, token_t t)
{
	char buf[1024];
	strncpy(buf, t.start, t.len);
	buf[t.len] = 0;
	fprintf(f, "%s", buf);
}

/* Convert token's string to integer */
long tok2int(token_t *t)
{
	char buf[1024];
	long i;
	strncpy(buf, t->start, t->len);
	buf[t->len] = 0;
	sscanf(buf, "%ld", &i);
	return i;
}

/* For hacks */
token_t* make_fake_tok(char *s)
{
	token_t *t = cgc_malloc(sizeof(token_t));
	if (!t)
		fail("cgc_malloc(sizeof(token_t))");
	t->start = cgc_malloc(strlen(s) + 1);
	strcpy(t->start, s);
	t->len = strlen(s);
	return t;
}


int is_add_op(char type)
{
	return type == TOK_PLUS || type == TOK_MINUS;
}

int is_mul_op(char type)
{
	return type == TOK_MUL || type == TOK_DIV;
}


//#include "gc.h"

/* Tree helper routines */

/* FIXME: crazy cgc_mallocs and memcpys, no garbage collection (!)
 *        a modern OS should clean up, but nevertheless, yeah */

//#include "tree.h"
//#include "tokenizer.h"
//#include "tokens.h"
//#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>



int sametree(exp_tree_t *a, exp_tree_t *b)
{
	int i;

	/* compare child counts and head-types ... */
	if (a->child_count != b->child_count)
		return 0;
	if (a->head_type != b->head_type)
		return 0;

	if (a->tok && b->tok) {
		/* extract token strings */
		strncpy(buf_a, a->tok->start, a->tok->len);
		buf_a[a->tok->len] = 0;
		strncpy(buf_b, b->tok->start, b->tok->len);
		buf_b[b->tok->len] = 0;

		/* compare 'em */
		if (strcmp(buf_a, buf_b))
			return 0;
	}

	/* recurse onto children */
	for (i = 0; i < a->child_count; ++i)
		if (!sametree(a->child[i], b->child[i]))
			return 0;

	/* none of all these tests failed ?
	 * the trees are the same, then ! */
	return 1;
}

exp_tree_t* copy_tree(exp_tree_t *src)
{
	exp_tree_t et = new_exp_tree(src->head_type, src->tok);
	int i;

	for (i = 0; i < src->child_count; ++i)
		if (src->child[i]->child_count == 0)
			add_child(&et, alloc_exptree(*src->child[i]));
		else
			add_child(&et, copy_tree(src->child[i]));

	return alloc_exptree(et);
}

exp_tree_t *alloc_exptree(exp_tree_t et)
{
	exp_tree_t *p = cgc_malloc(sizeof(exp_tree_t));
	if (!p)
		fail("cgc_malloc et");
	*p = et;
	return p;
}

int valid_tree(exp_tree_t et)
{
	return et.head_type != NULL_TREE;
}

exp_tree_t new_exp_tree(int type, token_t* tok)
{
	token_t* tok_copy = NULL;

	if (tok) {
		tok_copy = cgc_malloc(sizeof(token_t));
		if (!tok_copy)
			fail("cgc_malloc tok_copy");
		memcpy(tok_copy, tok, sizeof(token_t));
	}

	exp_tree_t tr;
	tr.head_type = type;
	tr.tok = tok_copy;
	tr.child_count = 0;
	tr.child_alloc = 64;
	tr.child = cgc_malloc(64 * sizeof(exp_tree_t *));
	if (!tr.child)
		fail("cgc_malloc tree children");
	tr.child_count = 0;

	return tr;
}

void add_child(exp_tree_t *dest, exp_tree_t* src)
{
	if (++dest->child_count >= dest->child_alloc) {
		dest->child_alloc += 64;
		dest->child = realloc(dest->child,
			dest->child_alloc * sizeof(exp_tree_t *));
		if (!dest->child)
			fail("realloc tree children");
	}
	dest->child[dest->child_count - 1] = src;
	dest->child[dest->child_count] = NULL;
}

/* 
 * Lisp-like prefix tree printout, 
 * useful for debugging the parser
 */
void printout_tree(exp_tree_t et)
{
	int i;
	/*
	fprintf(stderr, "(%s", tree_nam[et.head_type]);
	if (et.tok && et.tok->start) {
		fprintf(stderr, ":");
		tok_display(stderr, *et.tok);
	}
	for (i = 0; i < et.child_count; i++) {
		fprintf(stderr, " ");
		fflush(stderr);
		printout_tree(*(et.child[i]));
	}
	fprintf(stderr, ")");
	fflush(stderr);
	*/
}

int is_special_tree(int ht)
{
	return ((ht == MATCH_NUM)
	|| (ht == MATCH_VAR)
	|| (ht == MATCH_IND)
	|| (ht == MATCH_CONT)
	|| (ht == MATCH_FUNC)
	|| (ht == MATCH_CONTX)
	|| (ht == MATCH_NAN));
}

#ifndef APPLYRULES_H
#define APPLYRULES_H

int apply_rules_and_optimize(exp_tree_t** rules, int rc, exp_tree_t *t);

#endif
#ifndef DIAG_H
#define DIAG_H

void compiler_fail(char *message, token_t *token,
	int in_line, int in_chr);

#endif
#ifndef INFIXPRINTER_H
#define INFIXPRINTER_H
void printout_tree_infix(exp_tree_t* pet);
#endif
#ifndef MATCH_H
#define MATCH_H

int treematch(exp_tree_t *a, exp_tree_t *b, dict_t* d);
int matchloop(exp_tree_t** rules, int rc, exp_tree_t* tree);
exp_tree_t* find_subtree(exp_tree_t *tree, exp_tree_t *key);

#endif
#ifndef OPTIMIZE_H
#define OPTIMIZE_H

int optimize(exp_tree_t *et);

#endif

#ifndef RULEFILE_H
#define RULEFILE_H

void rule(char *r, exp_tree_t **rules, int* rc);
int readrules(exp_tree_t** rules, char *dir);

#endif
#ifndef SUBST_H
#define SUBST_H
void subst(exp_tree_t *tree, dict_t *d, exp_tree_t **father_ptr);
#endif




