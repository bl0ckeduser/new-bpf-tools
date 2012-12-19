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

	expr := ident asg-op expr | sum_expr [comp-op sum_expr]
			| 'int' ident [ = expr ]

	sum_expr := mul_expr { add-op mul_expr }+
	mul_expr := unary_expr { mul-op unary_expr }+

	unary_expr := ([ - ] ( ident | integer ) ) |  '(' expr ')' 
				| ident ++ | ident -- | ++ ident | -- ident
*/

token_t *tokens;
int index;

exp_tree_t block();
exp_tree_t expr();
exp_tree_t sum_expr();
exp_tree_t mul_expr();
exp_tree_t unary_expr();

token_t peek()
{
	return tokens[index];
}

token_t need(char type)
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
	} else
		return tokens[index - 1];
}

void parse(token_t *t)
{
	exp_tree_t program, subtree;
	program = new_exp_tree(BLOCK, NULL);

	tokens = t;
	index = 0;

	while (tokens[index].start) {
		subtree = block();
		if (!valid_tree(subtree))
			break;
		add_child(&program, subtree);
	}

	printout(program);

	putchar('\n');
}

void printout(exp_tree_t et)
{
	int i;
	printf("(%s", tree_nam[et.head_type]);
	for (i = 0; i < et.child_count; i++) {
		printf(" ");
		fflush(stdout);
		printout(et.child[i]);
	}
	printf(")");
	fflush(stdout);
}

exp_tree_t block()
{
	int i, args;
	exp_tree_t tree, subtree;
	token_t tok;

	/* empty block */
	if (peek().type == TOK_SEMICOLON) {
		++index;	/* eat semicolon */
		return new_exp_tree(BLOCK, NULL);
	}
	if (valid_tree(tree = expr())) {
		need(TOK_SEMICOLON);
		return tree;
	} else if(peek().type == TOK_IF) {
		++index;	/* eat if */
		tree = new_exp_tree(IF, NULL);
		need(TOK_LPAREN);
		add_child(&tree, expr());
		need(TOK_RPAREN);
		add_child(&tree, block());
		if (peek().type == TOK_ELSE) {
			++index;	/* eat else */
			add_child(&tree, block());
		}
		return tree;
	} else if(peek().type == TOK_WHILE) {
		++index;	/* eat while */
		tree = new_exp_tree(WHILE, NULL);
		need(TOK_LPAREN);
		add_child(&tree, expr());
		need(TOK_RPAREN);
		add_child(&tree, block());
		return tree;
	} else if(peek().type == TOK_LBRACE) {
		++index;	/* eat { */
		tree = new_exp_tree(BLOCK, NULL);
		while (valid_tree(subtree = block()))
			add_child(&tree, subtree);
		need(TOK_RBRACE);
		return tree;
	/* instr(arg1, arg2, argN); */
	} else if (is_instr(peek().type)) {
		tok = peek();
		tree = new_exp_tree(BPF_INSTR, &tok);
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
		++index;	/* eat instruction */
		need(TOK_LPAREN);
		for (i = 0; i < args; i++) {
			if (!valid_tree(subtree = expr()))
				fail("expression expected");
			add_child(&tree, subtree);
			if (i < args - 1)
				need(TOK_COMMA);
		}
		need(TOK_RPAREN);
		return tree;
	} else {
		return null_tree;
	}
}

exp_tree_t expr()
{
	exp_tree_t tree, subtree, subtree2;
	exp_tree_t subtree3, subtree4;
	token_t oper;
	token_t tok;

	if (peek().type == TOK_IDENT) {
		/* "ident asg-op expr" pattern */
		if(is_asg_op(tokens[index + 1].type)) {
			tree = new_exp_tree(ASGN, 0);
			tok = peek();
			subtree = new_exp_tree(VARIABLE, &tok);
			add_child(&tree, subtree);
			oper = tokens[index + 1];
			switch (oper.type) {
				case TOK_PLUSEQ:
					subtree = new_exp_tree(ADD, NULL);
					break;
				case TOK_MINUSEQ:
					subtree = new_exp_tree(SUB, NULL);
					break;
				case TOK_MULEQ:
					subtree = new_exp_tree(MULT, NULL);
					break;
				case TOK_DIVEQ:
					subtree = new_exp_tree(DIV, NULL);
					break;
				case TOK_ASGN:
					break;
				default:
					fail("invalid asg-op");
			}
			index += 2;	/* eat ident, asg-op */
			if (!valid_tree(subtree2 = expr()))
				fail("needed expression after ident asg-op");
			if (oper.type == TOK_ASGN)
				add_child(&tree, subtree2);
			else {
				add_child(&subtree, subtree2);
				add_child(&tree, subtree);
			}
			return tree;
		}
	}
	if (valid_tree(subtree = sum_expr())) {
		if (!is_comp_op(peek().type)) {
			return subtree;
		}
		if (is_comp_op(peek().type)) {
			switch (peek().type) {
				case TOK_GT:
					tree = new_exp_tree(GT, NULL);
					break;
				case TOK_GTE:
					tree = new_exp_tree(GTE, NULL);
					break;
				case TOK_LT:
					tree = new_exp_tree(LT, NULL);
					break;
				case TOK_LTE:
					tree = new_exp_tree(LTE, NULL);
					break;
				case TOK_EQ:
					tree = new_exp_tree(EQL, NULL);
					break;
				case TOK_NEQ:
					tree = new_exp_tree(NEQL, NULL);
					break;
				default:
					fail("invalid comparison operator");
			}
			++index;	/* eat comp-op */
			add_child(&tree, subtree);
			if (!valid_tree(subtree2 = sum_expr()))
				fail("sum_expr expected");
			add_child(&tree, subtree2);
		}
		return tree;
	}
	if(peek().type == TOK_INT) {
		++index;	/* eat int token */
		tree = new_exp_tree(INT_DECL, NULL);
		tok = need(TOK_IDENT);
		add_child(&tree, new_exp_tree(VARIABLE, &tok));
		if (peek().type == TOK_ASGN) {
			++index;	/* eat = */
			if (!valid_tree(subtree2 = expr()))
				fail("int ident = expr");
			add_child(&tree, subtree2);
		}
		return tree;
	}
	return null_tree;
}

exp_tree_t sum_expr()
{
	exp_tree_t tree, subtree, subtree2, root;
	token_t oper;

	if (!valid_tree(subtree = mul_expr())) {
		return null_tree;
	}
	if (!is_add_op(peek().type)) {
		return subtree;
	} else {
		oper = peek();
		switch (oper.type) {
			case TOK_PLUS:
				tree = new_exp_tree(ADD, NULL);
				break;
			case TOK_MINUS:
				tree = new_exp_tree(SUB, NULL);
				break;
			default:
				fail("invalid add-op");
		}
		++index;
		add_child(&tree, subtree);
	}
	root = tree;
	while (is_add_op(peek().type)) {
		oper = peek();
		switch (oper.type) {
			case TOK_PLUS:
				subtree = new_exp_tree(ADD, NULL);
				break;
			case TOK_MINUS:
				subtree = new_exp_tree(SUB, NULL);
				break;
			default:
				fail("invalid add-op");
		}
		add_child(&tree, subtree);
		++index;	/* eat add-op */
		if (!(valid_tree(subtree2 = mul_expr())))
			fail("sum_expr");
		add_child(&subtree, subtree2);
		tree = subtree;
	}
	return tree;
}


exp_tree_t mul_expr()
{
	exp_tree_t tree, subtree, subtree2, root;
	token_t oper;

	if (!valid_tree(subtree = unary_expr())) {
		return null_tree;
	}
	if (!is_mul_op(peek().type)) {
		return subtree;
	} else {
		oper = peek();
		switch (oper.type) {
			case TOK_MUL:
				tree = new_exp_tree(MULT, NULL);
				break;
			case TOK_DIV:
				tree = new_exp_tree(DIV, NULL);
				break;
			default:
				fail("invalid mul-op");
		}
		++index;
		add_child(&tree, subtree);
	}
	root = tree;
	while (is_mul_op(peek().type)) {
		oper = peek();
		switch (oper.type) {
			case TOK_MUL:
				tree = new_exp_tree(MULT, NULL);
				break;
			case TOK_DIV:
				tree = new_exp_tree(DIV, NULL);
				break;
			default:
				fail("invalid mul-op");
		}
		add_child(&tree, subtree);
		++index;	/* eat mul-op */
		if (!(valid_tree(subtree2 = unary_expr())))
			fail("unary_expr");
		add_child(&subtree, subtree2);
		tree = subtree;
	}
	return tree;
}

exp_tree_t unary_expr()
{
	exp_tree_t tree = null_tree, subtree = null_tree;
	exp_tree_t subtree2 = null_tree;
	token_t tok;

	if (peek().type == TOK_MINUS) {
		tree = new_exp_tree(NEGATIVE, NULL);
	}
	if (peek().type == TOK_PLUSPLUS
	||  peek().type == TOK_MINUSMINUS) {
		++index;	/* eat operator */
		tok = need(TOK_IDENT);
		subtree = new_exp_tree(VARIABLE, &tok);
		if (valid_tree(tree)) {
			add_child(&tree, subtree);
		} else
			tree = subtree;
		return tree;
	}

	tok = peek();
	if (tok.type == TOK_IDENT
	||  tok.type == TOK_INTEGER) {
		switch(tok.type) {
			case TOK_IDENT:
				subtree = new_exp_tree(VARIABLE, &tok);
				break;
			case TOK_INTEGER:
				subtree = new_exp_tree(NUMBER, &tok);
				break;
		}

		++index;	/* eat variable or number */

		if (tokens[index - 1].type == TOK_IDENT) {
			if (tok.type == TOK_PLUSPLUS) {
				++index;	/* eat ++ */
				subtree2 = new_exp_tree(INC, NULL);
				add_child(&subtree2, subtree);
			}
			if (tok.type == TOK_MINUSMINUS) {
				++index;	/* eat -- */
				subtree2 = new_exp_tree(DEC, NULL);
				add_child(&subtree2, subtree);
			}
		}

		if (valid_tree(subtree2)) {
			return subtree2;
		} else
			return subtree;
	}
	else if(peek().type == TOK_LPAREN) {
		++index;	/* eat ( */
		tree = expr();
		need(TOK_RPAREN);
		return tree;
	}
	return null_tree;
}
