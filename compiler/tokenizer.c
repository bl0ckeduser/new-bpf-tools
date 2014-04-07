/*
 * Tokenization code based on wannabe-regex.c,
 * which I wrote September-August 2012.
 * The tokenization is always "greedy".
 * It works using my "wannabe-regex" 
 * NFA pseudo-regex compiler/matcher.
 * 
 * On Aug. 31, 2013, the tokenization was
 * heavily modified: now the NFAs themselves
 * are always deterministic,
 * and consequently the matching code is cleaner
 * and faster.
 *
 * On Feb. 2014, keyword matching was updated to
 * use a hash table.
 *
 * Okay this is kind of messy, but what it boils down
 * to is: a graph whose edges are characters and nodes
 * "states", and where egde-crossing deterministically
 * leads you to a magic "state-node" that tells you what
 * token your little character edge-walk just made up.
 * (or if it doesn't then it's an error!)
 * It was far messier (and slower, etc) 
 * last year before I spent some time cleaning it up.
 *
 * Bl0ckeduser, December 2012 - updated Feb. 2014
 */

/* 
 * XXX: the automatons never get free()d
 */

#include "hashtable.h"
#include "tokenizer.h"
#include "tokens.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "general.h"
#include "diagnostics.h"

hashtab_t *keywords;
int *iptr;
char **code_lines;
int cl_alloc = 0;

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

	/* hash of match */
	unsigned int hash;
} match_t;

/*
 * NFA (compiled pattern) structure
 */
typedef struct nfa_struct {
	/* character edges */
	struct nfa_struct* map[256];

	/* epsilon edge */
	struct nfa_struct* link;

	/* accept state, if any (0 otherwise) */
	int valid_token;
} nfa;

/* ============================================ */

/* "nfa" structure routines */

nfa* new_nfa()
{
	nfa* t = malloc(sizeof(nfa));
	int i;
	if (!t)
		fail("nfa allocation");
	for(i = 0; i < 256; i++)
		t->map[i] = NULL;
	t->link = NULL;
	t->valid_token = 0;
	return t;
}

/* ============================================ */

/*
 * The wannabe-regex string => NFA compiler
 */

void add_token(nfa* t, char* tok, int key)
{
	int i, j;
	int c;
	nfa *hist[1024];
	nfa *next;
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
			case 'A':	/* set of all letters */
				next = new_nfa();
				for(j = 'a'; j <= 'z'; j++)
					t->map[j] = next;
				for(j = 'A'; j <= 'Z'; j++)
					t->map[j] = next;
				t->map['_'] = next;
				t = next;
				++n;
				break;
			case 'B':	/* set of all letters 
					   + all digits */
				next = new_nfa();
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
			case 'D':	/* set of all digits */
				next = new_nfa();
				for(j = '0'; j <= '9'; j++)
					t->map[j] = next;
				t = next;
				++n;
				break;
			case 'W':	/* whitespace */
				next = new_nfa();
				t->map[' '] = next;
				t->map['\t'] = next;
				t->map['\r'] = next; /* Macintosh / MS-DOS bullshit */
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
				hist[n - 1]->link = t;
				break;
			case '*':		/* 0 or more reps of a char */
			case '+':		/* 1 or more reps of a char */
				sanity_requires(n > 0);

				/* 
				 * Make a new "buffer" node to keep things
				 * cleanly separated and avoid
				 * surprises when matching e.g.
				 * 'a+b+'. Epsilon-link to it.
				 */
				t->link = hist[++n] = new_nfa();
				t = hist[n];

				/* 
				 * Repeat all the character edges from
				 * the node previous to the last character read,
				 * but this time make them go from the "buffer"
				 * node back to the node that came after the
				 * last character read.
				 */
				for (j = 0; j < 256; ++j)
					if (hist[n - 2]->map[j] == hist[n - 1])
						t->map[j] = hist[n - 1];

				/* 
				 * And finally make a clean
				 * new node for further work,
				 * and epsilon-link to it.
				 */
				t->link = hist[++n] = new_nfa();
				t = hist[n];

				if (c == '+')
					break;
				else {
					/* make epsilon for the 0-repetitions case in '*' */
					hist[n-3]->link = t;
					break;
				}
			case 'Q':		/* quotable chars */
				next = new_nfa();
				for(j = 0; j < 256; j++)
					if (j != '"')
						t->map[j] = next;
				t = next;
				++n;
				break;
			case '.':		/* all chars (well, except \n) */
				next = new_nfa();
				for(j = 0; j < 256; j++)
					if (j != '\n')
						t->map[j] = next;
				t = next;
				++n;
				break;
			case 'N':		/* \n */
				next = new_nfa();
				t->map['\n'] = next;
				t = next;
				++n;
				break;
			case '\\':	/* escape to normal text */
				if (++i == len)
					fail("backslash expected char");
				c = new_tok[i];
			default:	/* normal text */
				if (!t->map[c])
					t->map[c] = new_nfa();
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

/* 
 * nfa* t		:	matching automaton / current node
 * char* full_tok	:	full text
 * char* tok		: 	where we are now
 */	
match_t match(nfa* t, char* full_tok, char* tok)
{
	int i = 0;
	char c;
	int prev = -1;
	match_t m;
	int str_const = 0;
	unsigned int hash = 0;

	/* 
	 * Iterate the matching loop as long as there are
	 * characters left
	 */
	while (i <= strlen(tok)) {
		/* 
		 * The `c' variable is just being syntax sugar
		 * for "current character"
		 */
		c = tok[i];

		/*
		 * HACK to deal with \" in string constants
		 */
		if (c == '"')
			str_const = 1;
		if (c == '\\' && str_const) {
			i += 2;
			continue;
		}

		/* 
		 * If a character edge for the
		 * current character exists,
		 * follow it. 
		 */
		if (t->map[c]) {
			t = t->map[c];
			++i;

			/*
			 * incremental hashing (to spot keywords quickly;
			 * see later down in the file)
			 */
			if (prev == -1)
				prev = c;
			hash = hashtab_hash_char(c, prev, keywords->nbuck, hash);
			prev = c;
		}
		/* 
		 * Otherwise, check for an epsilon edge
		 * and follow it if it exists 
		 */
		else if (t->link) {
			t = t->link;
		/* 
		 * Otherwise: no epsilon, no character, no match;
		 * stop the loop
		 */
		} else
			break;

		if (t->valid_token)
			break;
	}

	/* 
	 * Set up the result-report structure
	 */
	if (t && t->valid_token) {
		/* Match succeeded */
		m.success = t->valid_token;
		m.pos = tok + i;
		m.hash = hash;
	} else {
		/* No match */
		m.success = 0;
	}


	return m;
}

nfa *t[100];
int tc = 0;

#define TOK_FAIL(msg)	\
	{ strcpy(fail_msg, msg); goto tok_fail; }

token_t* tokenize(char *buf)
{
	match_t m;
	match_t c;
	char buf2[1024]; /* XXX: assumes no token is >1023 chars */
	char fail_msg[1024] = "";
	char *p;
	char *old;
	int max;
	int i;
	int line = 1;
	char *line_start = buf;
	token_t *toks = malloc(4096 * sizeof(token_t));
	int tok_alloc = 64;
	int tok_count = 0;
	int comstat = NOT_INSIDE_A_COMMENT;

	cl_alloc = 64;
	code_lines = malloc(cl_alloc * sizeof(char *));
	*code_lines = buf;

	/*
	 * Build the keywords hash table
	 */
	keywords = new_hashtab();
	for (i = 0; i < KW_COUNT; ++i) {
		iptr = malloc(sizeof(int));
		if (!iptr)
			fail("couldn't get a few bytes of heap :(");
		*iptr = kw_tab[i].tok;
		hashtab_insert(keywords, kw_tab[i].str, iptr);
	}

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
		old = p;
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

		/*
		 * If inside a C comment, only look for
		 * comment close tokens. (otherwise some
		 * patterns will do nasty wrong things)
		 */
		if (comstat == INSIDE_A_C_COMMENT && m.success != C_CMNT_CLOSE) {
			max = -1;
			p = old;
		}

		if (max == -1) {
			/* 
			 * Matching from this offset failed
			 */
			if (comstat != NOT_INSIDE_A_COMMENT) {
				/* It's okay to have non-tokens in comments,
				 * so just go forward and suck it up */
				if (*p == '\n') {
					code_lines[line] = line_start;
					++line;
					if (line >= cl_alloc) {
						cl_alloc += 64;
						code_lines = realloc(code_lines, cl_alloc * sizeof(char *));
					}
					line_start = p;
				}
				++p;
				continue;
			}
			else
				/* Not in a comment, so tokenization has really failed */
				TOK_FAIL("tokenization failed -- unknown pattern");
		} else {
			/*
			 * Matching from this offset has succeeded
			 */

			/* 
			 * Spot keywords. They are initially
			 * recognized as identifiers, but we use
			 * a hash table (and inplace incremental 
			 * hashing in the matching routine, a hack 
			 * to save time) to discriminate them from
			 * identifiers.
			 */
			if (c.success == TOK_IDENT) {
				strncpy(buf2, p, max);
				buf2[max] = 0;
				if ((iptr = hashtab_lookup_with_hash(keywords, buf2, c.hash)))
					c.success = *iptr;
			}

			/*
			 * And now: comment stuff
			 */

			/*
			 * Open a C comment if not already in one
			 */
			if (c.success == C_CMNT_OPEN) {
				if (comstat == INSIDE_A_C_COMMENT)
					TOK_FAIL("Please do not nest C comments");
				comstat = INSIDE_A_C_COMMENT;
			/* 
			 * C comment closes are allowed if a C comment
			 * has already been opened
			 */
			} else if (c.success == C_CMNT_CLOSE) {
				if (comstat == INSIDE_A_C_COMMENT) {
					comstat = NOT_INSIDE_A_COMMENT;
					goto advance;
				} else {
					TOK_FAIL("Dafuq is a */ doing there ?");
				}
			/* 
			 * Start a C++ comment, unless already in a C comment
			 * (the string // inside a C comment is legal)
			 */
			} else if (c.success == CPP_CMNT) {
				if (comstat != INSIDE_A_C_COMMENT)
					comstat = INSIDE_A_CPP_COMMENT;	
			}
			/* 
			 * C++ comments end at the end of their line
			 */
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
				if (++tok_count >= tok_alloc) {
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

				/*
				 * Ignore preprocessor directives,
				 * like http://bellard.org/otcc/ does
				 * in order to have "stdout" and "stderr"
				 * and still fake C compatibility
				 */
				if (c.success == TOK_CPP) {
					code_lines[line] = line_start;
					compiler_warn("preprocessor directive consumed and ignored",
						&toks[tok_count - 1], 0, 0);
					--tok_count;
				}
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
			if (c.success == TOK_NEWLINE || c.success == TOK_CPP) {
				code_lines[line] = line_start;
				++line;
				if (line >= cl_alloc) {
					cl_alloc += 64;
					code_lines = realloc(code_lines, cl_alloc * sizeof(char *));
				}
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

	tc = 3;
	for (i = 0; i < 3; ++i)
		t[i] = new_nfa();

	/* 
	 * there are two NFAs: t[0] and t[1].
	 * this separation is required because
	 * the NFA compiler and matcher can't
	 * deal with ambiguities such as '&'
	 * vs. '&&', so e.g. '&' would go t[0]
	 * and '&&' in t[1], and then the main
	 * matching loop would choose the longest
	 * match between t[0] and t[1].
	 * Ideally, I should come up with some kind
	 * of automatic way of solving this kind of thing.
	 * Anyway, this like 10x faster than one NFA
	 * per token !
	 * 
	 * Not sure how space efficient this is
	 * ("mildly" I guess. surely the upper bound is a few
	 * megabytes, which is not *so* bad in 2013)
	 *
	 * Update Oct 2013: now there's three of them
	 */

	/* D: any digit; B: letter or digit */
	add_token(t[0], "0D+", TOK_OCTAL_INTEGER);
	add_token(t[1], "0xB+", TOK_HEX_INTEGER);
	add_token(t[1], "0XB+", TOK_HEX_INTEGER);
	add_token(t[2], "D+", TOK_INTEGER);

	/* W: any whitespace character */
	add_token(t[0], "W+", TOK_WHITESPACE);

	/* 
	 * Preprocessor directive.
	 * Maybe I could instead have a pattern
	 * for each directive or whatever,
	 * e.g. maybe something like
	 * "#defineW*AB*W*.*N" for defines
	 */
	add_token(t[1], "#.*N", TOK_CPP);

	/* A: letter; B: letter or digit */
	add_token(t[0], "AB*", TOK_IDENT);

	/* operators */
	add_token(t[0], "\\+", TOK_PLUS);
	add_token(t[0], "-", TOK_MINUS);
	add_token(t[0], "/", TOK_DIV);
	add_token(t[0], "\\*", TOK_MUL);
	add_token(t[0], "=", TOK_ASGN);
	add_token(t[1], "==", TOK_EQ);
	add_token(t[0], ">", TOK_GT);
	add_token(t[0], "<", TOK_LT);
	add_token(t[1], ">=", TOK_GTE);
	add_token(t[1], "<=", TOK_LTE);
	add_token(t[2], "<<", TOK_LSHIFT);
	add_token(t[2], ">>", TOK_RSHIFT);
	add_token(t[1], "!=", TOK_NEQ);
	add_token(t[1], "\\+=", TOK_PLUSEQ);
	add_token(t[1], "-=", TOK_MINUSEQ);
	add_token(t[1], "/=", TOK_DIVEQ);
	add_token(t[1], "\\*=", TOK_MULEQ);
	add_token(t[1], "\\+\\+", TOK_PLUSPLUS);
	add_token(t[1], "--", TOK_MINUSMINUS);
	add_token(t[0], "%", TOK_MOD);
	add_token(t[0], "&", TOK_ADDR);
	add_token(t[1], "&&", TOK_CC_AND);
	add_token(t[2], "&=", TOK_BAND_EQ);
	add_token(t[1], "%=", TOK_MODEQ);
	add_token(t[0], "||", TOK_CC_OR);
	add_token(t[1], "|", TOK_PIPE);
	add_token(t[2], "|=", TOK_BOR_EQ);
	add_token(t[0], "!", TOK_CC_NOT);
	add_token(t[0], "\\.", TOK_DOT);
	add_token(t[1], "->", TOK_ARROW);
	add_token(t[0], "\\?", TOK_QMARK);
	add_token(t[0], "\\^", TOK_CARET);
	add_token(t[1], "\\^=", TOK_BXOR_EQ);

	/* special characters */
	add_token(t[0], "{", TOK_LBRACE);
	add_token(t[0], "}", TOK_RBRACE);
	add_token(t[0], "(", TOK_LPAREN);
	add_token(t[0], ")", TOK_RPAREN);
	add_token(t[0], ";", TOK_SEMICOLON);
	add_token(t[0], ",", TOK_COMMA);
	add_token(t[0], "\n", TOK_NEWLINE);
	add_token(t[0], ":", TOK_COLON);
	add_token(t[0], "[", TOK_LBRACK);
	add_token(t[0], "]", TOK_RBRACK);

	/* comments */
	add_token(t[2], "/\\*", C_CMNT_OPEN);
	add_token(t[2], "\\*/", C_CMNT_CLOSE);
	add_token(t[2], "//", CPP_CMNT);

	/* string constants */
	add_token(t[0], "\"Q*\"", TOK_STR_CONST);

	/* character constant with an escape in it */
	add_token(t[0], "'\\\\.'", TOK_CHAR_CONST_ESC);

	/* character constant */
	add_token(t[1], "'.'", TOK_CHAR_CONST);
}

