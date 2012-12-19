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

extern token_t* tokenize(char *string);
extern void setup_tokenizer();

#endif
