/* Token helper routines */
/* pretty stupid dumb boring asinine stuff here */

#include "tokens.h"
#include <string.h>
#include <stdio.h>

char* tok_nam[] = {
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
	"TOK_CPP",
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

char* tok_desc[] = {
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

/* keywords */

struct bpf_kw kw_tab[] = 
{
	{ "if", TOK_IF },
	{ "while", TOK_WHILE },
	{ "break", TOK_BREAK },
	{ "int", TOK_INT },
	{ "void", TOK_VOID },
	{ "char", TOK_CHAR },
	{ "echo", TOK_ECHO },
	{ "draw", TOK_DRAW },
	{ "wait", TOK_WAIT },
	{ "cmx", TOK_CMX },
	{ "cmy", TOK_CMY },
	{ "mx", TOK_MX },
	{ "my", TOK_MY },
	{ "outputdraw", TOK_OD },
	{ "else", TOK_ELSE },
	{ "goto", TOK_GOTO },
	{ "return", TOK_RET },
	{ "for", TOK_FOR },
	{ "struct", TOK_STRUCT },
	{ "typedef", TOK_TYPEDEF },
	{ "sizeof", TOK_SIZEOF },
	{ "long", TOK_LONG },
	{ "continue", TOK_CONTINUE },
	{ "switch", TOK_SWITCH },
	{ "case", TOK_CASE },
	{ "default", TOK_DEFAULT },
	{ "enum", TOK_ENUM },
	{ "extern", TOK_EXTERN }
};

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

