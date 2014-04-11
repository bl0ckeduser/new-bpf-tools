#ifndef TOKENS_H
#define TOKENS_H

#include "tokenizer.h"
#include <stdlib.h>
#include <stdio.h>

/* token types */

enum {
	TOK_IF = 0,
	TOK_WHILE,
	TOK_INT,
	TOK_ECHO,
	TOK_DRAW,
	TOK_WAIT,
	TOK_CMX,
	TOK_CMY,
	TOK_MX,
	TOK_MY,
	TOK_OD,
	TOK_INTEGER,
	TOK_PLUS,
	TOK_MINUS,
	TOK_DIV,
	TOK_MUL,
	TOK_ASGN,
	TOK_EQ,
	TOK_GT,
	TOK_LT,
	TOK_GTE,
	TOK_LTE,
	TOK_NEQ,
	TOK_PLUSEQ,
	TOK_MINUSEQ,
	TOK_DIVEQ,
	TOK_MULEQ,
	TOK_WHITESPACE,
	TOK_IDENT,
	TOK_PLUSPLUS,
	TOK_MINUSMINUS,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_LPAREN,
	TOK_RPAREN,
	TOK_SEMICOLON,
	TOK_ELSE,
	TOK_COMMA,
	TOK_NEWLINE,
	TOK_GOTO,
	TOK_COLON,
	TOK_LBRACK,
	TOK_RBRACK,
	C_CMNT_OPEN,
	C_CMNT_CLOSE,
	CPP_CMNT,
	TOK_PROC,
	TOK_RET,
	TOK_FOR,
	TOK_ADDR,
	TOK_STR_CONST,
	TOK_MOD,
	TOK_MODEQ,
	TOK_DEREF,
	TOK_CC_OR,
	TOK_CC_AND,
	TOK_CC_NOT,
	TOK_OCTAL_INTEGER,
	TOK_HEX_INTEGER,
	TOK_CHAR_CONST,
	TOK_CHAR_CONST_ESC,
	TOK_NOMORETOKENSLEFT,
	TOK_CHAR,
	TOK_BREAK,
	TOK_STRUCT,
	TOK_DOT,
	TOK_TYPEDEF,
	TOK_ARROW,
	TOK_SIZEOF,
	TOK_LONG,
	TOK_QMARK,
	TOK_CPP,
	TOK_CARET,
	TOK_PIPE,
	TOK_LSHIFT,
	TOK_RSHIFT,
	TOK_BAND_EQ,
	TOK_BOR_EQ,
	TOK_BXOR_EQ,
	TOK_CONTINUE,
	TOK_SWITCH,
	TOK_CASE,
	TOK_DEFAULT,
	TOK_VOID,
	TOK_ENUM,
	TOK_EXTERN
};

#define KW_COUNT 28

struct bpf_kw {
	char *str;
	char tok;
};

extern char** tok_nam;
extern char** tok_desc;
extern struct bpf_kw *kw_tab;

/* routines */

int is_add_op(char type);
int is_asg_op(char type);
int is_mul_op(char type);
int is_comp_op(char type);
int is_shift_op(char type);
int is_basic_type(char type);
int is_instr(char type);
void tok_display(token_t t);

#endif

