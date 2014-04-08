#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "hashtable.h"

typedef struct token {
	char type;
	char* start;
	int len;

	/* for elegant parser diagnostics */
	int from_line;	/* originating source line */
	int from_char;	/* originating offset in that line */
} token_t;

extern char **code_lines;

extern token_t* tokenize(char *string, hashtab_t *cpp_defines);
extern void setup_tokenizer();

#endif
