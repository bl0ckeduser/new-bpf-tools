/*
 * Parser for new BPF compiler
 * Bl0ckeduser, December 2012
 */

#include <stdio.h>
#include <stdlib.h>
#include "tokenizer.h"
#include "tokens.h"
#include "tree.h"
#include <string.h>

/*
	block := expr ';' | if (expr) block [else block] 
			| while (expr) block | '{' { expr ';' } '}' 
			| instr ( expr1, expr2, exprN ) ';'
			| ident ':'
			| goto ident ';'

	lvalue := ident [ '[' expr ']' ]

	expr := lvalue asg-op expr | sum_expr [comp-op sum_expr]
			| 'int' ident [ (= expr) | ( '[' integer ']' ) ]

	sum_expr := mul_expr { add-op mul_expr }+
	mul_expr := unary_expr { mul-op unary_expr }+

	unary_expr := ([ - ] ( lvalue | integer | unary_expr ) ) 
				|  '(' expr ')' | lvalue ++ | lvalue --
				| ++ lvalue | -- lvalue
*/

token_t *tokens;
int indx;
int tok_count;

exp_tree_t block();
exp_tree_t expr();
exp_tree_t sum_expr();
exp_tree_t mul_expr();
exp_tree_t unary_expr();
void printout(exp_tree_t et);
extern void fail(char* mesg);

token_t peek()
{
	if (indx >= tok_count)
		return tokens[tok_count - 1];
	else
		return tokens[indx];
}

#define need(X) need_call((X), __LINE__)

void parse_fail(char *message)
{
	token_t tok;
	int line, chr;
	extern void compiler_fail(char *message,
		 token_t *token, int in_line, int in_chr);

	if (tokens[indx].from_line
		!= tokens[indx - 1].from_line) {
		/* end of the line */
		line = tokens[indx - 1].from_line;
		chr = strstr(code_lines[line], "\n")
			- code_lines[line] + 1;
	} else {
		line = tokens[indx].from_line;
		chr = tokens[indx].from_char;
	}

	compiler_fail(message, NULL, line, chr);
}

token_t need_call(char type, int source_line)
{
	char buf[1024];
	if (tokens[indx++].type != type) {
		fflush(stdout);
		printf("\n");
		--indx;
		sprintf(buf, "%s expected", tok_desc[type]);
		parse_fail(buf);
	} else
		return tokens[indx - 1];
}

exp_tree_t parse(token_t *t)
{
	exp_tree_t program;
	exp_tree_t *subtree;
	program = new_exp_tree(BLOCK, NULL);
	int i;

	for (i = 0; t[i].start; ++i)
		;

	tok_count = i;

	tokens = t;
	indx = 0;

	while (tokens[indx].start) {
		subtree = alloc_exptree(block());
		if (!valid_tree(*subtree))
			break;
		add_child(&program, subtree);
	}

	return program;
}

exp_tree_t lval()
{
	token_t tok = peek();
	exp_tree_t tree, subtree;

	if (tok.type == TOK_IDENT) {
		/* array expression: identifier[index] */
		if(tokens[indx + 1].type == TOK_LBRACK) {
			tree = new_exp_tree(ARRAY, NULL);
			add_child(&tree,
				alloc_exptree(new_exp_tree(VARIABLE, &tok)));
			indx += 2;	/* eat the identifier and the [ */
			if (!valid_tree(subtree = expr()))
				parse_fail("expression expected");
			add_child(&tree, alloc_exptree(subtree));
			need(TOK_RBRACK);
			return tree;	
		/* identifier alone */		
		} else {
			++indx;	/* eat the identifier */
			return new_exp_tree(VARIABLE, &tok);
		}
	} else
		return null_tree;
}

exp_tree_t block()
{
	int i, args;
	exp_tree_t tree, subtree;
	token_t tok;

	/* empty block */
	if (peek().type == TOK_SEMICOLON) {
		++indx;	/* eat semicolon */
		return new_exp_tree(BLOCK, NULL);
	}
	/* label ? */
	if (peek().type == TOK_IDENT) {
		tok = peek();
		tree = new_exp_tree(LABEL, &tok);
		++indx;	/* eat label name */
		if (peek().type == TOK_COLON) {
			++indx;	/* eat : */
			return tree;
		}
		else --indx;
	}
	if (valid_tree(tree = expr())) {
		need(TOK_SEMICOLON);
		return tree;
	} else if(peek().type == TOK_IF) {
		++indx;	/* eat if */
		tree = new_exp_tree(IF, NULL);
		need(TOK_LPAREN);
		add_child(&tree, alloc_exptree(expr()));
		need(TOK_RPAREN);
		add_child(&tree, alloc_exptree(block()));
		if (peek().type == TOK_ELSE) {
			++indx;	/* eat else */
			add_child(&tree, alloc_exptree(block()));
		}
		return tree;
	} else if(peek().type == TOK_WHILE) {
		++indx;	/* eat while */
		tree = new_exp_tree(WHILE, NULL);
		need(TOK_LPAREN);
		add_child(&tree, alloc_exptree(expr()));
		need(TOK_RPAREN);
		add_child(&tree, alloc_exptree(block()));
		return tree;
	} else if(peek().type == TOK_LBRACE) {
		++indx;	/* eat { */
		tree = new_exp_tree(BLOCK, NULL);
		while (valid_tree(subtree = block()))
			add_child(&tree, alloc_exptree(subtree));
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
				parse_fail("illegal BPF instruction");
		}
		++indx;	/* eat instruction */
		need(TOK_LPAREN);
		for (i = 0; i < args; i++) {
			if (!valid_tree(subtree = expr()))
				parse_fail("expression expected");
			add_child(&tree, alloc_exptree(subtree));
			if (i < args - 1)
				need(TOK_COMMA);
		}
		need(TOK_RPAREN);
		need(TOK_SEMICOLON);
		return tree;
	} else if (peek().type == TOK_GOTO) {
		++indx;	/* eat goto */
		tok = need(TOK_IDENT);
		return new_exp_tree(GOTO, &tok);
	} else {
		return null_tree;
	}
}

exp_tree_t expr()
{
	exp_tree_t tree, subtree, subtree2;
	exp_tree_t subtree3;
	exp_tree_t lv;
	token_t oper;
	token_t tok;
	int save = indx;

	if (valid_tree(lv = lval())) {
		/* "lvalue asg-op expr" pattern */
		if(is_asg_op(peek().type)) {
			tree = new_exp_tree(ASGN, NULL);
			add_child(&tree, alloc_exptree(lv));
			oper = peek();
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
					parse_fail("invalid assignment operator");
			}
			indx++;	/* eat asg-op */
			if (!valid_tree(subtree3 = expr()))
				parse_fail("expression expected");
			if (oper.type == TOK_ASGN)
				add_child(&tree, alloc_exptree(subtree3));
			else {
				add_child(&subtree, alloc_exptree(lv));
				add_child(&subtree, alloc_exptree(subtree3));
				add_child(&tree, alloc_exptree(subtree));
			}
			return tree;
		} else
			indx = save;
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
					parse_fail("invalid comparison operator");
			}
			++indx;	/* eat comp-op */
			add_child(&tree, alloc_exptree(subtree));
			if (!valid_tree(subtree2 = sum_expr()))
				parse_fail("expression expected");
			add_child(&tree, alloc_exptree(subtree2));
		}
		return tree;
	}
	if(peek().type == TOK_INT) {
		++indx;	/* eat int token */
		tree = new_exp_tree(INT_DECL, NULL);
		tok = need(TOK_IDENT);
		subtree = new_exp_tree(VARIABLE, &tok);
		add_child(&tree, alloc_exptree(subtree));
		if (peek().type == TOK_ASGN) {
			++indx;	/* eat = */
			if (!valid_tree(subtree2 = expr()))
				parse_fail("expected expression after =");
			add_child(&tree, alloc_exptree(subtree2));
		} else if(peek().type == TOK_LBRACK) {
			++indx;	/* eat [ */
			tok = need(TOK_INTEGER);
			need(TOK_RBRACK);
			add_child(&tree, 
				alloc_exptree(new_exp_tree(NUMBER, &tok)));
			tree.head_type = ARRAY_DECL;
		}
		return tree;
	}
	return null_tree;
}

/* difficult part */
exp_tree_t sum_expr()
{
	exp_tree_t et1, et2;
	exp_tree_t *subtree, *subtree2, *tree, *root;
	token_t oper;

	if (!valid_tree(et1 = mul_expr()))
		return null_tree;
	
	if (!is_add_op((oper = peek()).type))
		return et1;
	
	subtree = alloc_exptree(et1);
	
	switch (oper.type) {
		case TOK_PLUS:
			et2 = new_exp_tree(ADD, NULL);
		break;
		case TOK_MINUS:
			et2 = new_exp_tree(SUB, NULL);
		break;
	}
	++indx;	/* eat add-op */
	root = tree = alloc_exptree(et2);
	add_child(tree, subtree);

	while (1) {
		if (!valid_tree(et1 = mul_expr()))
			parse_fail("expression expected");

		if (!is_add_op((oper = peek()).type)) {
			add_child(tree, alloc_exptree(et1));
			return *root;
		}
		switch (oper.type) {
			case TOK_PLUS:
				et2 = new_exp_tree(ADD, NULL);
			break;
			case TOK_MINUS:
				et2 = new_exp_tree(SUB, NULL);
			break;
		}
		++indx;	/* eat add-op */

		subtree = alloc_exptree(et2);
		subtree2 = alloc_exptree(et1);
		add_child(subtree, subtree2);
		add_child(tree, subtree);

		tree = subtree;
		subtree = subtree2;
	}

	return *root;
}

/* mostly a repeat of sum_expr */
exp_tree_t mul_expr()
{
	exp_tree_t et1, et2;
	exp_tree_t *subtree, *subtree2, *tree, *root;
	token_t oper;

	if (!valid_tree(et1 = unary_expr()))
		return null_tree;
	
	if (!is_mul_op((oper = peek()).type))
		return et1;
	
	subtree = alloc_exptree(et1);
	
	switch (oper.type) {
		case TOK_DIV:
			et2 = new_exp_tree(DIV, NULL);
		break;
		case TOK_MUL:
			et2 = new_exp_tree(MULT, NULL);
		break;
	}
	++indx;	/* eat add-op */
	root = tree = alloc_exptree(et2);
	add_child(tree, subtree);

	while (1) {
		if (!valid_tree(et1 = unary_expr()))
			parse_fail("expression expected");

		if (!is_mul_op((oper = peek()).type)) {
			add_child(tree, alloc_exptree(et1));
			return *root;
		}
		switch (oper.type) {
			case TOK_DIV:
				et2 = new_exp_tree(DIV, NULL);
			break;
			case TOK_MUL:
				et2 = new_exp_tree(MULT, NULL);
			break;
		}
		++indx;	/* eat add-op */

		subtree = alloc_exptree(et2);
		subtree2 = alloc_exptree(et1);
		add_child(subtree, subtree2);
		add_child(tree, subtree);

		tree = subtree;
		subtree = subtree2;
	}

	return *root;
}

exp_tree_t unary_expr()
{
	exp_tree_t tree = null_tree, subtree = null_tree;
	exp_tree_t subtree2 = null_tree, subtree3;
	token_t tok;

	if (peek().type == TOK_MINUS) {
		tree = new_exp_tree(NEGATIVE, NULL);
		++indx;	/* eat sign */
	}

	if (peek().type == TOK_PLUSPLUS
	||  peek().type == TOK_MINUSMINUS) {
		switch (peek().type) {
			case TOK_PLUSPLUS:
				subtree = new_exp_tree(INC, NULL);
				break;
			case TOK_MINUSMINUS:
				subtree = new_exp_tree(DEC, NULL);
				break;
			}
			++indx;	/* eat operator */
			if (!valid_tree(subtree2 = lval()))
				parse_fail("lvalue expected");
			add_child(&subtree, alloc_exptree(subtree2));	
		if (valid_tree(tree)) {
			add_child(&tree, alloc_exptree(subtree));
		} else
			tree = subtree;
		return tree;
	}

	tok = peek();
	if (valid_tree(subtree = lval())
	||  tok.type == TOK_INTEGER) {

		if (tok.type == TOK_INTEGER) {
				subtree = new_exp_tree(NUMBER, &tok);
				++indx;	/* eat number */
		} else {
			if (peek().type == TOK_PLUSPLUS) {
				++indx;	/* eat ++ */
				subtree2 = new_exp_tree(POST_INC, NULL);
				add_child(&subtree2, alloc_exptree(subtree));
			}
			if (peek().type == TOK_MINUSMINUS) {
				++indx;	/* eat -- */
				subtree2 = new_exp_tree(POST_DEC, NULL);
				add_child(&subtree2, alloc_exptree(subtree));
			}
		}

		if (valid_tree(subtree2))
			subtree3 = subtree2;
		else
			subtree3 = subtree;

		if (valid_tree(tree)) {
			add_child(&tree, alloc_exptree(subtree3));
		} else
			tree = subtree3;

		return tree;
	}
	if(peek().type == TOK_LPAREN) {
		++indx;	/* eat ( */
		subtree = expr();
		need(TOK_RPAREN);
		
		if (valid_tree(tree))
			add_child(&tree, alloc_exptree(subtree));
		else
			tree = subtree;
		
		return tree;
	}

	/* last option: - (unary_expr) */
	if (valid_tree(tree)) {
		if (valid_tree(subtree = unary_expr())) {
			add_child(&tree, alloc_exptree(subtree));
			return tree;
		}
	}

	return null_tree;
}
