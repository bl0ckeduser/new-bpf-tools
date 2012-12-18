/*
 * Tokenization code based on wannabe-regex.c,
 * which I wrote last September-August. The old file
 * has more comments, some different features, 
 * and additional bugs. The tokenization is
 * "greedy".
 *
 * Bl0ckeduser, December 2012
 */

/* TODO: cleanup some of the more mysterious
 *	     parts of the code;
 *		 token.from_line, token.from_char
 */

#include "tokenizer.h"
#include "tokens.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	int i;

	if (++from->links > from->link_alloc)
		from->link = realloc(from->link, 
			(from->link_alloc += 5) * sizeof(trie *));

	if (!from->link)
		fail("link array allocation");

	from->link[from->links - 1] = to;
}

trie* new_trie()
{
	trie* t = malloc(sizeof(trie));
	int i;
	if (!t)
		fail("trie allocation");
	t->link = malloc(10 * sizeof(trie*));
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

int add_token(trie* t, char* tok, int key)
{
	int i, j;
	int k;
	int c;
	trie *hist[1024];
	trie *next;
	int len = strlen(tok);
	char* new_tok;
	int n;

	new_tok = malloc(len + 32);
	if (!new_tok)
		fail("buffer allocation");
	strcpy(new_tok, tok);

	for (i = 0, n = 0; c = new_tok[i], i < len && t; ++i)
	{
		hist[n] = t;

		switch (c) {
			case 'A':		/* special: set of all letters */
				next = new_trie();
				for(j = 'a'; j <= 'z'; j++)
					t->map[j] = next;
				for(j = 'A'; j <= 'Z'; j++)
					t->map[j] = next;
				t = next;
				++n;
				break;
			case 'B':		/* special: set of all letters 
							   + all digits */
				next = new_trie();
				for(j = 'a'; j <= 'z'; j++)
					t->map[j] = next;
				for(j = 'A'; j <= 'Z'; j++)
					t->map[j] = next;
				for(j = '0'; j <= '9'; j++)
					t->map[j] = next;
				t = next;
				++n;
				break;
			case 'D':		/* special: set of all digits */
				next = new_trie();
				for(j = '0'; j <= '9'; j++)
					t->map[j] = next;
				t = next;
				++n;
				break;
			case 'W':		/* special: whitespace */
				next = new_trie();
				t->map[' '] = next;
				t->map['\t'] = next;
				t->map['\n'] = next;
				t = next;
				++n;
				break;
			case '?':		/* optional character */
				sanity_requires(n > 0);

				/* 
				 * The previous node is allowed to
				 * go straight to this node, via
				 * an epsilon link.
				 */
				add_link(hist[n - 1], t);
				break;
			case '+':		/* character can repeat
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
			case '*':		/* optional character
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
				add_link(hist[n - 1], t);

				break;
			case '\\':		/* escape to normal text */
				if (++i == len)
					fail("backslash expected char");
				c = new_tok[i];
			default:		/* normal text */
				if (!t->map[c])
					t->map[c] = new_trie();
				t = t->map[c];
				++n;
		}
	}

	free(new_tok);

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

match_t match(trie* automaton, char* full_text, char* where)
{
	extern match_t match_algo(trie* t, char* full_tok, char* tok,
		int btm, int vd, trie* led, int ledi);

	return match_algo(automaton, full_text, where, 0, 0, NULL, 0);
}

/* trie* t				:	matching automaton
 * char* full_tok		:	full text
 * char* tok			: 	where we are now
 * int btm				: 	internal. initially, give 0
 * int vd				:	internal, initially, give 0
 * trie* led			:	internal. start with NULL
 * int ledi				:	internal. start with 0
 */	
match_t match_algo(trie* t, char* full_tok, char* tok, int btm, 
	int vd, trie* led, int ledi)
{
	int i = 0;
	int j;
	char c;

	/* greedy choice algorithm */
	int record;			/* longest match */
	match_t m;			/* match register */
	match_t ml[10];		/* match list */
	int mc = 0;			/* match count */
	match_t* choice;	/* final choice */	

	int len = strlen(tok) + 1;
	int init = btm;

	int last_inc;

	i = 0; do
	{
		last_inc = 0;
		c = tok[i];

		if (t->links && !btm)
		{
			/*
			 * Ambiguous. Try all possibilites by "forking"
			 * (informal sense).
			 */

			/* Try character edge if it exists */
			if (t->map[c])
				if ((m = match_algo(t, full_tok, tok + i, 1,
					vd + 1,	led, ledi)).token)
					ml[mc++] = m;

			/* Try all the epsilon nodes */
			for (j = 0; j < t->links; j++)
				if ((m = match_algo(t, full_tok, tok + i, j + 2, 
					vd + 1, led, ledi)).token)
					ml[mc++] = m;

			/*
			 * If there are multiple matches,
			 * be "greedy" (choose longest match).
			 */
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
			else if (mc == 1) /* No ambiguity, no time wasted 
								  choosing */
				return *ml;
			else	/* For non-matches (they are never pushed
					   to the decision list) */
				return m;
		}

		if (btm <= 1 && t->map[c]) {
			t = t->map[c];

			last_inc = 1;

			if (t->valid_token)
				break;
			
			++i;
			goto valid;
		}
		else if (btm > 1) {
			/* prevents infinite loops */
			if (!(led && t->link[btm - 2] == led
			    && (int)(&tok[i] - full_tok) == ledi)) {

				/* t->links == 1 is special */
				if (t->links > 1) {
					led = t;	/* Last Epsilon departure
							       node */
					ledi = (int)(&tok[i] - full_tok);
				}

				t = t->link[btm - 2];
					
				goto valid;
			}
		}

		break;

		/* The fork path choice is only
		 * relevant for the first ambiguous
		 * choice after the fork
		 */
valid:	if (btm)
			btm = 0;

	} while (i < len && t);

	if (t && t->valid_token) {
		m.token = t->valid_token;
		m.pos = tok + i + last_inc;
	} else {
		m.token = 0;
	}

	return m;
}

trie *t[100];
int tc = 0;

token_t* tokenize(char *buf)
{
	match_t m;
	match_t c;
	char buf2[1024];
	char *p;
	int max;
	int key;
	int i;

	token_t *toks = malloc(64 * sizeof(token_t));
	int tok_alloc = 64;
	int tok_count = 0;

	for (p = buf; *p;) {
		max = -1;
		key = 0;

		/* 
		 * Choose the matching pattern
		 * that gives the longest match.
		 * Shit is "meta-greedy".
		 */
		for (i = 0; i < tc; i++) {
			m = match(t[i], buf, p);
			if (m.token) {	/* pattern matched */
				if (m.pos - p > max) {
					c = m;
					max = m.pos - p;
					key = tc;
				}
			}
		}

		if (max == -1) {
			/* matching from this offset failed */
			printf("dafuq: %s\n", p);
			break;	/* stop tokenizing, bye bye */
		} else {
			/* spot keywords */
			if (c.token == TOK_IDENT) {
				strncpy(buf2, p, max);
				for (i = 0; i < KW_COUNT; i++) {
					if (!strcmp(buf2, kw_tab[i].str))
						c.token = kw_tab[i].tok;
				}
			}

			/* add token to the tokens array */
			if (c.token != TOK_WHITESPACE) {
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
			}

			/* move forward in the string */
			p += max;
			if (max == 0)
				++p;
		}
	}

	toks[tok_count].start = NULL;
	toks[tok_count].len = 0;

	return toks;
}

void setup_tokenizer()
{
	int i = 0;

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
	add_token(t[tc++], "==", TOK_EQ);
	add_token(t[tc++], ">", TOK_GT);
	add_token(t[tc++], "<", TOK_LT);
	add_token(t[tc++], ">=", TOK_GTE);
	add_token(t[tc++], "<=", TOK_LTE);
	add_token(t[tc++], "!=", TOK_NEQ);
	add_token(t[tc++], "\\+=", TOK_PLUSEQ);
	add_token(t[tc++], "-=", TOK_MINUSEQ);
	add_token(t[tc++], "/=", TOK_DIVEQ);
	add_token(t[tc++], "\\*=", TOK_MULEQ);
	add_token(t[tc++], "\\+\\+", TOK_PLUSPLUS);
	add_token(t[tc++], "--", TOK_MINUSMINUS);

	/* special characters */
	add_token(t[tc++], "{", TOK_LBRACE);
	add_token(t[tc++], "}", TOK_RBRACE);
	add_token(t[tc++], "(", TOK_LPAREN);
	add_token(t[tc++], ")", TOK_RPAREN);
	add_token(t[tc++], ";", TOK_SEMICOLON);
	add_token(t[tc++], ",", TOK_COMMA);
}

