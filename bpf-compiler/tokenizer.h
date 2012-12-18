#ifndef TOKENIZER_H
#define TOKENIZER_H

typedef struct token {
	char type;
	char* start;
	int len;

	/* for elegant parser diagnostics */
	int from_line;
	int from_char;
} token_t;

#endif
