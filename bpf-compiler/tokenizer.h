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

extern char *code_lines[1024];

extern token_t* tokenize(char *string);
extern void setup_tokenizer();

#endif
