#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "hashtable.h"

typedef struct token {
	char type;
	char* start;
	int len;

	/* for elegant parser diagnostics */
	char* from_line;	/* originating source line (linemark string) */
	int from_char;	/* originating offset in that line */
	char *lineptr;
} token_t;

extern char **code_lines;

token_t* tokenize(char *string, hashtab_t *cpp_defines);
void setup_tokenizer();

#endif
