/*
 * Parser for new BPF compiler
 * Bl0ckeduser, December 2012
 */

/* TODO:
 *		- figure out a spec for the syntax-trees
 *		- produce real syntax trees rather than printouts
 */

/*

	Binary trees would probably
	be a safe bet.
	Can convert to N-ary afterwards.

		=
	/		\
   x		+					
		  /	  \						
		  1    *
			  /  \			
			  2   3		
*/

#include <stdio.h>
#include <stdlib.h>
#include "tokenizer.h"
#include "tokens.h"
#include "tree.h"

/*
	block := expr ';' | if (expr) block [else block] 
			| while (expr) block | '{' { expr ';' } '}' 
			| instr ( expr1, expr2, exprN )

	expr := ident asg-op expr | sum_expr {comp-op sum_expr} 
			| 'int' ident [ = expr ]

	sum_expr := mul_expr { add-op mul_expr }+
	mul_expr := unary_expr { mul-op unary_expr }+

	unary_expr := ([ - ] ( ident | integer ) ) |  '(' expr ')' 
				| ident ++ | ident -- | ++ ident | -- ident
*/

token_t *tokens;
int index;

token_t peek()
{
	return tokens[index];
}

void need(char type)
{
	char buf[1024];
	if (tokens[index++].type != type) {
		fflush(stdout);
		printf("\n");
		sprintf(buf, "at index %d, needed a %s",
			index - 1, tok_nam[type]);
		/* token index --> line, char 	array
		 * would allow cool diagnostics like this:
		 *		1 + 1
		 *			 ^
		 * Syntax error: semicolon expected
		 */
		
		fail(buf);
	}
}

void parse(token_t *t)
{
	exp_tree_t program;
	program = new_exp_tree(BLOCK, 0);

	tokens = t;
	index = 0;


	while (block()
		&& tokens[index].start)
		;

	putchar('\n');
}

int block()
{
	int i;
	int args;

	printf("(block ");
	/* empty block */
	if (peek().type == TOK_SEMICOLON) {
		printf(" semicolon)");
		++index;
		return 1;
	}
	if (expr()) {
		need(TOK_SEMICOLON);
		printf(" semicolon)");
		return 1;
	} else if(peek().type == TOK_IF) {
		printf(" (if ");
		++index;
		need(TOK_LPAREN);
		expr();
		need(TOK_RPAREN);
		block();
		if (peek().type == TOK_ELSE) {
			printf(" else ");
			++index;
			block();
		}
		printf("))");
		return 1;
	} else if(peek().type == TOK_WHILE) {
		printf(" (while ");
		++index;
		need(TOK_LPAREN);
		expr();
		need(TOK_RPAREN);
		block();
		printf("))");
		return 1;
	} else if(peek().type == TOK_LBRACE) {
		++index;
		printf(" '{' ");
		while (block())
			;
		need(TOK_RBRACE);
		printf(" '}' )");
		return 1;
	} else if (is_instr(peek().type)) {
		printf(" (%s ", tok_nam[peek().type]);
		switch(peek().type) {
			case TOK_ECHO:
			case TOK_CMX:
			case TOK_CMY:
			case TOK_MX:
			case TOK_MY:
				args = 1;
				break;
			case TOK_DRAW:
				args = 8;
				break;
			case TOK_WAIT:
				args = 2;
				break;
			case TOK_OD:
				args = 0;
				break;
			default:
				fail("illegal BPF instruction");
		}
		++index;
		/* parenthesized, comma-separated
	 	 * parameter-expression list */
		need(TOK_LPAREN);
		for (i = 0; i < args; i++) {
			if (!expr())
				fail("expression expected");
			if (i < args - 1)
				need(TOK_COMMA);
		}
		need(TOK_RPAREN);
		printf("))");
		return 1;
	} else {
		printf("]");
		return 0;
	}
}

int expr()
{
	printf("(expr ");
	if (peek().type == TOK_IDENT) {
		/* "ident asg-op expr" pattern */
		if(is_asg_op(tokens[index + 1].type)) {
			index += 2;
			printf("TOK_IDENT ASG-OP ");
			if (!expr())
				fail("needed expression after ident asg-op");
			printf(")");
			return 1;
		}
	}
	if (sum_expr()) {
		if (!is_comp_op(peek().type)) {
			printf(")");
			return 1;
		}
		while (is_comp_op(peek().type)) {
			printf(" comp-op ");
			++index;
			if (!sum_expr())
				fail("sum_expr expected");
		}
		printf(")");
		return 1;
	}
	if(peek().type == TOK_INT) {
		printf("int ");
		++index;
		need(TOK_IDENT);
		printf("TOK_IDENT ");
		if (peek().type == TOK_ASGN) {
			printf("= ");
			++index;
			if (!expr())
				fail("int ident = expr");
		}
		printf(")");
		return 1;
	}
	printf("]");
	return 0;
}

int sum_expr()
{
	printf("(sum_expr ");
	if (!mul_expr()) {
		printf("]");
		return 0;
	}
	if (!is_add_op(peek().type)) {
		printf(")");
		return 1;
	}
	while (is_add_op(peek().type)) {
		printf(" add-op ");
		++index;
		if (!mul_expr())
			fail("sum_expr");
	}
	printf(")");
	return 1;
}


int mul_expr()
{
	printf("(mul_expr ");
	if (!unary_expr()) {
		printf("]");
		return 0;
	}
	if (!is_mul_op(peek().type)) {
		printf(")");
		return 1;
	}
	while (is_mul_op(peek().type)) {
		printf(" mul_op ");
		++index;
		if (!unary_expr())
			fail("unary_expr needed");
	}
	printf(")");
	return 1;
}

int unary_expr()
{
	printf("(unary_expr ");
	if (peek().type == TOK_MINUS) {
		printf(" - ");
	}
	if (peek().type == TOK_PLUSPLUS
	||  peek().type == TOK_MINUSMINUS) {
		printf("%s ", tok_nam[peek().type]);
		++index;
		need(TOK_IDENT);
		printf("TOK_IDENT ");
		printf(")");
		return 1;
	}
	if (peek().type == TOK_IDENT
	||  peek().type == TOK_INTEGER) {
		printf("%s", tok_nam[peek().type]);
		++index;

		if (tokens[index - 1].type == TOK_IDENT) {
			if (peek().type == TOK_PLUSPLUS
			||  peek().type == TOK_MINUSMINUS) {
				printf(" %s", tok_nam[peek().type]);
				++index;
			}
		}

		printf(")");
		return 1;
	}
	else if(peek().type == TOK_LPAREN) {
		printf(" '(' ");
		++index;
		expr();
		need(TOK_RPAREN);
		printf(" ')' ");
		printf(")");
		return 1;
	}
	printf("]");
	return 0;
}
