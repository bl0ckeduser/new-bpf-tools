/* Token helper routines */
/* pretty stupid dumb boring asinine stuff here */

#include "tokens.h"
#include <string.h>
#include <stdio.h>

int kwno = 0;
char** tok_nam;
char** tok_desc;
struct bpf_kw *kw_tab;

void kwtab_append(char *k, int v)
{
	struct bpf_kw entry;
	entry.str = k;
	entry.tok = v;
	kw_tab[kwno++] = entry;
}

void init_tokens()
{
	int i;
	char* tok_nam_local[] = {
		"TOK_IF",
		"TOK_WHILE",
		"TOK_INT",
		"TOK_ECHO",
		"TOK_DRAW",
		"TOK_WAIT",
		"TOK_CMX",
		"TOK_CMY",
		"TOK_MX",
		"TOK_MY",
		"TOK_OD",
		"TOK_INTEGER",
		"TOK_PLUS",
		"TOK_MINUS",
		"TOK_DIV",
		"TOK_MUL",
		"TOK_ASGN",
		"TOK_EQ",
		"TOK_GT",
		"TOK_LT",
		"TOK_GTE",
		"TOK_LTE",
		"TOK_NEQ",
		"TOK_PLUSEQ",
		"TOK_MINUSEQ",
		"TOK_DIVEQ",
		"TOK_MULEQ",
		"TOK_WHITESPACE",
		"TOK_IDENT",
		"TOK_PLUSPLUS",
		"TOK_MINUSMINUS",
		"TOK_LBRACE",
		"TOK_RBRACE",
		"TOK_LPAREN",
		"TOK_RPAREN",
		"TOK_SEMICOLON",
		"TOK_ELSE",
		"TOK_COMMA",
		"TOK_NEWLINE",
		"TOK_GOTO",
		"TOK_COLON",
		"TOK_LBRACK",
		"TOK_RBRACK",
		"C_CMNT_OPEN",
		"C_CMNT_CLOSE",
		"CPP_CMNT",
		"TOK_PROC",
		"TOK_RET",
		"TOK_FOR",
		"TOK_ADDR",
		"TOK_STR_CONST",
		"TOK_MOD",
		"TOK_MODEQ",
		"TOK_DEREF",
		"TOK_CC_OR",
		"TOK_CC_AND",
		"TOK_CC_NOT",
		"TOK_OCTAL_INTEGER",
		"TOK_HEX_INTEGER",
		"TOK_CHAR_CONST",
		"TOK_CHAR_CONST_ESC",
		"TOK_NOMORETOKENSLEFT",
		"TOK_CHAR",
		"TOK_BREAK",
		"TOK_STRUCT",
		"TOK_DOT",
		"TOK_TYPEDEF",
		"TOK_ARROW",
		"TOK_SIZEOF",
		"TOK_LONG",
		"TOK_QMARK",
		"TOK_LINEMARK",
		"TOK_CARET",
		"TOK_PIPE",
		"TOK_LSHIFT",
		"TOK_RSHIFT",
		"TOK_BAND_EQ",
		"TOK_BOR_EQ",
		"TOK_BXOR_EQ",
		"TOK_CONTINUE",
		"TOK_SWITCH",
		"TOK_CASE",
		"TOK_DEFAULT",
		"TOK_VOID",
		"TOK_ENUM",
		"TOK_EXTERN"
	};

	tok_nam = malloc(sizeof(tok_nam_local));
	for (i = 0; i < sizeof(tok_nam_local) / sizeof(char *); ++i)
		tok_nam[i] = tok_nam_local[i];

	char* tok_desc_local[] = {
		"if",
		"while",
		"int declaration",
		"echo instruction",
		"draw instruction",
		"wait instruction",
		"cmx instruction",
		"cmy instruction",
		"mx instruction",
		"my instruction",
		"outputdraw instruction",
		"integer",
		"+",
		"-",
		"/",
		"*",
		"=",
		"==",
		">",
		"<",
		">=",
		"<=",
		"!=",
		"+=",
		"-=",
		"/=",
		"*=",
		"whitepsace",
		"identifier",
		"++",
		"--",
		"{",
		"}",
		"(",
		")",
		";",
		"else",
		",",
		"newline",
		"goto",
		":",
		"[",
		"]",
		"/*",
		"*/",
		"//",
		"proc",
		"return",
		"for",
		"address-of",
		"string constant",
		"%",
		"%=",
		"dereference",
		"||",
		"&&",
		"!",
		"octal numeral",
		"hexadecimal numeral",
		"character constant",
		"character constant",
		"nothing",
		"char",
		"break",
		"struct",
		"->",
		"sizeof",
		"long",
		"?",
		"preprocessor directive",
		"^",
		"|",
		"<<",
		">>",
		"continue",
		"switch",
		"switch case label",
		"switch default label",
		"void type",
		"enum",
		"extern"
	};

	tok_desc = malloc(sizeof(tok_desc_local));
	for (i = 0; i < sizeof(tok_desc_local) / sizeof(char *); ++i)
		tok_desc[i] = tok_desc_local[i];

	kw_tab = malloc(KW_COUNT * sizeof(struct bpf_kw));

	kwtab_append( "if", TOK_IF );
	kwtab_append( "while", TOK_WHILE );
	kwtab_append( "break", TOK_BREAK );
	kwtab_append( "int", TOK_INT );
	kwtab_append( "void", TOK_VOID );
	kwtab_append( "char", TOK_CHAR );
	kwtab_append( "echo", TOK_ECHO );
	kwtab_append( "draw", TOK_DRAW );
	kwtab_append( "wait", TOK_WAIT );
	kwtab_append( "cmx", TOK_CMX );
	kwtab_append( "cmy", TOK_CMY );
	kwtab_append( "mx", TOK_MX );
	kwtab_append( "my", TOK_MY );
	kwtab_append( "outputdraw", TOK_OD );
	kwtab_append( "else", TOK_ELSE );
	kwtab_append( "goto", TOK_GOTO );
	kwtab_append( "return", TOK_RET );
	kwtab_append( "for", TOK_FOR );
	kwtab_append( "struct", TOK_STRUCT );
	kwtab_append( "typedef", TOK_TYPEDEF );
	kwtab_append( "sizeof", TOK_SIZEOF );
	kwtab_append( "long", TOK_LONG );
	kwtab_append( "continue", TOK_CONTINUE );
	kwtab_append( "switch", TOK_SWITCH );
	kwtab_append( "case", TOK_CASE );
	kwtab_append( "default", TOK_DEFAULT );
	kwtab_append( "enum", TOK_ENUM );
	kwtab_append( "extern", TOK_EXTERN );
}

/* =================================== */

/* Print out a token's raw string */
void tok_display(token_t t)
{
	char buf[1024];
	strncpy(buf, t.start, t.len);
	buf[t.len] = 0;
	fprintf(stderr, "%s", buf);
}

int is_add_op(char type)
{
	return type == TOK_PLUS || type == TOK_MINUS;
}

int is_asg_op(char type)
{
	return type == TOK_PLUSEQ
 		|| type == TOK_MINUSEQ
		|| type == TOK_DIVEQ
		|| type == TOK_MULEQ
		|| type == TOK_MODEQ
		|| type == TOK_ASGN
		|| type == TOK_BOR_EQ
		|| type == TOK_BAND_EQ
		|| type == TOK_BXOR_EQ;
}

int is_mul_op(char type)
{
	return type == TOK_MUL || type == TOK_DIV
			|| type == TOK_MOD;
}

int is_comp_op(char type)
{
	return type == TOK_GT 
		|| type == TOK_LT
		|| type == TOK_GTE
		|| type == TOK_LTE
		|| type == TOK_NEQ
		|| type == TOK_EQ;
}

int is_shift_op(char type)
{
	return type == TOK_LSHIFT
		|| type == TOK_RSHIFT;
}

int is_basic_type(char type)
{
	return type == TOK_INT
			|| type == TOK_CHAR
			|| type == TOK_LONG
			|| type == TOK_VOID;
}

int is_instr(char type)
{
	return type == TOK_ECHO 
		|| type == TOK_DRAW
		|| type == TOK_WAIT
	 	|| type == TOK_CMX
		|| type == TOK_CMY
		|| type == TOK_MX
		|| type == TOK_MY
		|| type == TOK_OD;
}

