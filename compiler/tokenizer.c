/*
 * Tokenization code based on wannabe-regex.c,
 * which I wrote September-August 2012.
 * The tokenization is always "greedy".
 * It works using my "wannabe-regex" 
 * NFA pseudo-regex compiler/matcher.
 *
 * Bl0ckeduser, December 2012 - updated August 2013
 */

/* 
 * XXX: the automatons never get free()d
 */

#include "tokenizer.h"
#include "tokens.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *code_lines[1024];
extern void fail(char* mesg);
extern void sanity_requires(int exp);
extern void compiler_fail(char *message, token_t *token,
	int in_line, int in_chr);

enum {
	NOT_INSIDE_A_COMMENT,
	INSIDE_A_C_COMMENT,
	INSIDE_A_CPP_COMMENT
};

/* ============================================ */

/* 
 * Result reporting structure for the NFA matcher
 */
typedef struct match_struct {
	int success;
	
	/* pointer to last character matched */
	char* pos;
} match_t;

/*
 * NFA (compiled pattern) structure
 */
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

/* "Trie" (i.e. actually NFA used for regex) routines */

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

/*
 * The wannabe-regex string => NFA compiler
 */

void add_token(trie* t, char* tok, int key)
{
	int i, j;
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
			case 'Q':		/* quotable chars */
				next = new_trie();
				for(j = 0; j < 256; j++)
					if (j != '"')
						t->map[j] = next;
				t = next;
				++n;
				break;
			case '.':		/* all chars */
				next = new_trie();
				for(j = 0; j < 256; j++)
					t->map[j] = next;
				t = next;
				++n;
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

	free(new_tok);

	/* 
	 * We have reached the accept-state node;
	 * mark it as such.
	 */
	t->valid_token = key;
}


/*
 * wannabe-regex NFA matcher
 */

match_t match(trie* automaton, char* full_text, char* where)
{
	extern match_t match_algo(trie* t, char* full_tok, char* tok,
		int frk, trie* led, int ledi);

	return match_algo(automaton, full_text, where, 0, NULL, 0);
}

/* 
 * trie* t		:	matching automaton / current node
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

	i = 0; do
	{
		last_inc = 0;
		c = tok[i];

		/* 
		 * Check if multiple epsilon edges exist
		 * and if we are not already in a fork
		 */ 
		if (t->links && !frk)
		{
			/* 
			 * Fork for each possibility
			 * and collect results in an array
			 */

			/* character edge: frk = 1 */
			if (t->map[c])
				if ((m = match_algo(t, full_tok, tok + i, 1,
					led, ledi)).success)
					ml[mc++] = m;

			/* epsilon edge(s): frk = 2 + link number */
			for (j = 0; j < t->links; j++)
				if ((m = match_algo(t, full_tok, tok + i, 
					j + 2, led, ledi)).success)
					ml[mc++] = m;

			/* 
			 * If there are multiple matches, be "greedy",
			 * i.e. choose the longest match.
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
			else if (mc == 1) /* only one match */
				return *ml;
			else	/* no match, return blank as-is */
				return m;
		}

		/* 
		 * If we are not in an explicit 
		 * epsilon-edge-following fork
		 * and the character edge exists,
		 * follow it. 
		 */
		if (frk <= 1 && t->map[c]) {
			t = t->map[c];

			last_inc = 1;

			/* Break loop if in matching state */
			if (t->valid_token)
				break;
			
			++i;
			goto valid;
		}
		else if (frk > 1) {
			/* 
			 * This prevents infinite loops: if the
			 * epsilon chosen in this fork brings us back
			 * to the last epsilon followed prior to a fork
			 * and the character index has not increased
			 * since we followed this last epsilon, do not
			 * follow it. 
			 */
			if (!(led && t->link[frk - 2] == led
			    && (int)(&tok[i] - full_tok) == ledi)) {

				/*
				 * If t->links == 1, then there is
				 * no epsilon-choice ambiguity, and
			 	 * therefore no forking is going
				 * to happen, so the whole infinite
				 * loops hack doesn't apply.
				 */
				if (t->links > 1) {
					/* last epsilon departure node */
					led = t;
					/* last epsilon departure index */
					ledi = (int)(&tok[i] - full_tok);
				}

				/* 
				 * Okay the crazy part is done,
				 * now just follow the epsilon
				 * specified by the fork number
				 */
				t = t->link[frk - 2];
					
				goto valid;
			}
		}

		break;

valid:	
		/* 
		 * Clear the fork path number (if any) after
		 * the first iteration of the loop --
	 	 * all it determines is the choice in the
		 * first iteration of the loop, not anything
		 * else.
		 */
		if (frk)
			frk = 0;

	} while (i < len && t);

	/* 
	 * Set up the result-report structure
	 */
	if (t && t->valid_token) {
		/* Match succeeded */
		m.success = t->valid_token;
		m.pos = tok + i + last_inc;
	} else {
		/* No match */
		m.success = 0;
	}

	return m;
}

trie *t[100];
int tc = 0;

#define TOK_FAIL(msg)	\
	{ strcpy(fail_msg, msg); goto tok_fail; }

token_t* tokenize(char *buf)
{
	match_t m;
	match_t c;
	char buf2[1024];
	char fail_msg[1024] = "";
	char *p;
	int max;
	int i;
	int line = 1;
	char *line_start = buf;

	token_t *toks = malloc(4096 * sizeof(token_t));
	int tok_alloc = 64;
	int tok_count = 0;
	int comstat = NOT_INSIDE_A_COMMENT;

	*code_lines = buf;

	/* While there are characters left ... */
	for (p = buf; *p;) {
		/* 
		 * Choose the matching pattern
		 * on the code-string from this offset
		 * that gives the longest match.
		 * One might observe that 
		 * "shit is meta-greedy",
		 * because the the pattern-matcher is 
		 * greedy for ambiguities within a 
		 * single given pattern, and now
		 * this loop is being greedy about
		 * which pattern to choose.
		 */
		max = -1;	/* clear record length */
		for (i = 0; i < tc; i++) {
			/* try matching pattern number i */
			m = match(t[i], buf, p);
			/* check if match has succeeded */
			if (m.success) {
				/* if match length > record length */
				if (m.pos - p > max) {
					/* update record length to this one */
					max = m.pos - p;
					/* store this match as the new key */
					c = m;
				}
			}
		}

		if (max == -1) {
			/* 
			 * Matching from this offset failed
			 */
			if (comstat != NOT_INSIDE_A_COMMENT) {
				/* It's okay to have non-tokens in comments,
				 * so just go forward and suck it up */
				++p;
				continue;
			}
			else
				/* Not in a comment, so tokenization has really failed */
				TOK_FAIL("tokenization failed -- unknown pattern");
		} else {
			/*
			 * Matching from this offset has suceeded
			 */

			/* 
			 * Spot keywords. They are initially
			 * recognized as identifiers. 
			 */
			if (c.success == TOK_IDENT) {
				strncpy(buf2, p, max);
				buf2[max] = 0;
				for (i = 0; i < KW_COUNT; i++) {
					if (!strcmp(buf2, kw_tab[i].str))
						c.success = kw_tab[i].tok;
				}
			}

			/*
			 * Deal with comment stuff
			 */
			/* Open a C comment if not already in one */
			if (c.success == C_CMNT_OPEN) {
				if (comstat == INSIDE_A_C_COMMENT)
					TOK_FAIL("Please do not nest C comments");
				comstat = INSIDE_A_C_COMMENT;
			/* C comment closes are allowed if a C comment
			 * has already been opened */
			} else if (c.success == C_CMNT_CLOSE) {
				if (comstat == INSIDE_A_C_COMMENT) {
					comstat = NOT_INSIDE_A_COMMENT;
					goto advance;
				} else {
					TOK_FAIL("Dafuq is a */ doing there ?");
				}
			/* Start a C++ comment, unless already in a C comment */
			} else if (c.success == CPP_CMNT) {
				if (comstat == INSIDE_A_C_COMMENT)
					TOK_FAIL("Don't mix and nest comments, please");
				comstat = INSIDE_A_CPP_COMMENT;	
			}
			/* C++ comments end at the end of their line */
			if (comstat == INSIDE_A_CPP_COMMENT
				&& c.success == TOK_NEWLINE) {
				comstat = NOT_INSIDE_A_COMMENT;
				goto advance;
			}

			/* 
			 * Add token to the tokens array
			 * if it's not inside a comment
			 * and it's not useless whitespace
			 */
			if (c.success != TOK_WHITESPACE
			 && c.success != TOK_NEWLINE
			 && comstat == NOT_INSIDE_A_COMMENT) {
				/* Expand token array if necessary */
				if (++tok_count > tok_alloc) {
					tok_alloc = tok_count + 64;
					toks = realloc(toks,
						tok_alloc * sizeof(token_t));
					if (!toks)
						fail("resize tokens array");
				}
			
				/* 
				 * Build the token pointer stuff,
				 * and also add a bunch of debug tracing
				 * crap
				 */
				toks[tok_count - 1].type = c.success;
				toks[tok_count - 1].start = p;
				toks[tok_count - 1].len = max;
				toks[tok_count - 1].from_line = line;
				toks[tok_count - 1].from_char = p - 
					line_start + 1;
			}

advance:
			/* 
			 * Move forward in the string by the length
			 * of the match, or otherwise by 1 character
			 */
			p += max;
			if (max == 0)
				++p;

			/* 
			 * Line accounting (useful for
		 	 * pretty parse-fail diagnostics)
			 */
			if (c.success == TOK_NEWLINE) {
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

tok_fail:
	code_lines[line] = line_start;
	compiler_fail(fail_msg,
		NULL,
		line, p - line_start + 1);
}

/* 
 * Set up the wannabe-regex automatons
 * that match the various tokens. Maybe
 * this could be rewritten as an array
 * initializer...
 */
void setup_tokenizer()
{
	int i = 0;

	for (i = 0; i < 100; i++)
		t[i] = new_trie();

	/* D: any digit; B: letter or digit */
	add_token(t[tc++], "0D+", TOK_OCTAL_INTEGER);
	add_token(t[tc++], "0xB+", TOK_HEX_INTEGER);
	add_token(t[tc++], "0XB+", TOK_HEX_INTEGER);
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
	add_token(t[tc++], "&", TOK_ADDR);
	add_token(t[tc++], "%", TOK_MOD);
	add_token(t[tc++], "%=", TOK_MODEQ);
	add_token(t[tc++], "||", TOK_CC_OR);
	add_token(t[tc++], "&&", TOK_CC_AND);
	add_token(t[tc++], "!", TOK_CC_NOT);

	/* special characters */
	add_token(t[tc++], "{", TOK_LBRACE);
	add_token(t[tc++], "}", TOK_RBRACE);
	add_token(t[tc++], "(", TOK_LPAREN);
	add_token(t[tc++], ")", TOK_RPAREN);
	add_token(t[tc++], ";", TOK_SEMICOLON);
	add_token(t[tc++], ",", TOK_COMMA);
	add_token(t[tc++], "\n", TOK_NEWLINE);
	add_token(t[tc++], ":", TOK_COLON);
	add_token(t[tc++], "[", TOK_LBRACK);
	add_token(t[tc++], "]", TOK_RBRACK);

	/* comments */
	add_token(t[tc++], "/\\*", C_CMNT_OPEN);
	add_token(t[tc++], "\\*/", C_CMNT_CLOSE);
	add_token(t[tc++], "//", CPP_CMNT);

	/* string constants */
	add_token(t[tc++], "\"Q*\"", TOK_STR_CONST);

	/* character constants */
	add_token(t[tc++], "'.'", TOK_CHAR_CONST);
}

