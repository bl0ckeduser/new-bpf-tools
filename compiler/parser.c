/* Parser (tokens -> tree) */
/* See also: grammar.txt */

#include <stdio.h>
#include <stdlib.h>
#include "tokenizer.h"
#include "tokens.h"
#include "tree.h"
#include <string.h>

extern int escape_code(char c);

token_t *tokens;
int indx;
int tok_count;

char typedef_tag[32][64];
exp_tree_t typedef_desc[64];
int typedefs = 0;

extern char* get_tok_str(token_t t);

exp_tree_t block();
exp_tree_t expr();
exp_tree_t sum_expr();
exp_tree_t mul_expr();
exp_tree_t ternary_expr();
exp_tree_t ccor_expr();
exp_tree_t ccand_expr();
exp_tree_t comp_expr();
exp_tree_t decl();
exp_tree_t decl2();
exp_tree_t cast();
exp_tree_t cast_type();
exp_tree_t arg();
exp_tree_t struct_decl();
exp_tree_t e1();
exp_tree_t e0();
exp_tree_t e0_2();
exp_tree_t e0_3();
exp_tree_t e0_4();
exp_tree_t initializer();
int decl_dispatch(char type);

void printout(exp_tree_t et);
extern void fail(char* mesg);

int check_typedef(char *str)
{
	int i;
	for (i = 0; i < typedefs; ++i)
		if (!strcmp(typedef_tag[i], str))
			return 1;
	return 0;
}

exp_tree_t copy_tree(exp_tree_t src_a)
{
	exp_tree_t *src = &src_a;
    exp_tree_t et = new_exp_tree(src->head_type, src->tok);
    int i;

    for (i = 0; i < src->child_count; ++i)
            if (src->child[i]->child_count == 0)
                    add_child(&et, alloc_exptree(*src->child[i]));
            else
                    add_child(&et, alloc_exptree(copy_tree(*(src->child[i]))));

    return *alloc_exptree(et);
}

int indx_bound(int indx)
{
	if (indx > tok_count)
		indx = tok_count;
	return indx;
}

int adv()
{
	return indx_bound(++indx);
}

/* give the current token */
token_t peek()
{
	return tokens[indx];
}

/* generate a pretty error when parsing fails.
 * this is a hack-wrapper around the routine
 * in diagnostics.c that deals with certain
 * special cases special to the parser. */
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

/*
 * Fail if the next token isn't of the desired type.
 * Otherwise, return it and advance.
 */
token_t need(char type)
{
	char buf[1024];
	if (tokens[adv() - 1].type != type) {
		fflush(stdout);
		--indx;
		sprintf(buf, "%s expected", tok_desc[type]);
		parse_fail(buf);
	} else
		return tokens[indx - 1];
}

/* main parsing loop */
exp_tree_t parse(token_t *t)
{
	exp_tree_t program;
	exp_tree_t *subtree;
	program = new_exp_tree(BLOCK, NULL);
	token_t end = { TOK_NOMORETOKENSLEFT, "", 0, 0, 0 };
	int i;

	/* count the tokens */
	for (i = 0; t[i].start; ++i)
		;
	t[tok_count = i] = end;

	/* the loop */
	tokens = t;
	indx = 0;
	while (tokens[indx].start) {
		subtree = alloc_exptree(block());
		/* parse fails at EOF */
		if (!valid_tree(*subtree))
			break;
		add_child(&program, subtree);
	}

	return program;
}

/* 
 * Now the real fun starts. Each routine is
 * named after the part of the grammar that
 * it deals with. When it doesn't find anything
 * that it's been trained to find, a routine
 * gives back "null_tree". "indx" is a global
 * variable giving the current token index.
 * Don't ask why, but using a variable called
 * "index" made some compilers on some systems
 * very unhappy.
 */

/*
 * cast := '(' cast-type ')' e0
 */
exp_tree_t cast()
{
	exp_tree_t tree, ct, ue;
	int restor;

	if (peek().type == TOK_LPAREN) {
		restor = adv() - 1;	/* eat '(' */
		if (valid_tree(ct = cast_type())) {
			/* okay, it has to be a cast */
			tree = new_exp_tree(CAST, NULL);
			add_child(&tree, alloc_exptree(ct));
			need(TOK_RPAREN);	/* eat and check ')' */
			if (!valid_tree(ue = e1()))
				parse_fail("unary expression expected after cast");
			add_child(&tree, alloc_exptree(ue));
			return tree;
		} else
			indx = restor;	/* backtrack and leave */
	}
	return null_tree;
}

/*
 * cast-type := basic-type {'*'} | <typedef-tag> {'*'}
 *				| 'struct' ident {'*'}
 */
exp_tree_t cast_type()
{
	token_t bt;
	exp_tree_t btt, btct, ct, star;
	char *str = get_tok_str(peek());
	int i, sav_indx;

	/* 'struct' ident ... */
	if (peek().type == TOK_STRUCT) {
		sav_indx = adv() - 1;
		bt = peek();
		adv();
		if (bt.type != TOK_IDENT || peek().type == TOK_LBRACE) {
			indx = sav_indx;
		} else {
			btct = new_exp_tree(NAMED_STRUCT_DECL, &bt);
			goto cast_typedef;
		}
	}

	for (i = 0; i < typedefs; ++i) {
		if (!strcmp(typedef_tag[i], str)) {
			adv();
			btct = copy_tree(typedef_desc[i]);
			if (btct.head_type == CAST_TYPE) {
				ct = btct;
				goto cast_typedef_2;
			}
			goto cast_typedef;
		}
	}

	if (is_basic_type((bt = peek()).type)) {
		adv();			/* eat basic-type */
		btct = new_exp_tree(decl_dispatch(bt.type), NULL);
cast_typedef:
		ct = new_exp_tree(CAST_TYPE, NULL);
		btt = new_exp_tree(BASE_TYPE, NULL);
		add_child(&btt, alloc_exptree(btct));
		add_child(&ct, alloc_exptree(btt));
		star = new_exp_tree(DECL_STAR, NULL);
cast_typedef_2:
		while (peek().type == TOK_MUL)
			adv(), add_child(&ct, alloc_exptree(star));
		return ct;
	} else if(valid_tree((btt = struct_decl())))
		return btt;

	return null_tree;
}

/* 
 * decl := (basic-type | struct-decl) decl2 { ','  decl2 } 
 */
int decl_dispatch(char type)
{
	switch (type) {
		case TOK_INT:
			return INT_DECL;
		break;
		case TOK_LONG:
			return LONG_DECL;
		break;
		case TOK_CHAR:
			return CHAR_DECL;
		break;
	}
}
int decl_dedispatch(char type)
{
	switch (type) {
		case INT_DECL:
			return TOK_INT;
		break;
		case LONG_DECL:
			return TOK_LONG;
		break;
		case CHAR_DECL:
			return TOK_CHAR;
		break;
	}
}
exp_tree_t decl()
{
	token_t tok = peek();
	exp_tree_t tree, subtree;
	char *str = get_tok_str(peek());
	int i, id;
	int td_decl = 0;

	/* 
	 * (basic-type|struct-decl|<typedef-tag>) decl2 { ','  decl2 }
  	 */
	for (i = 0; i < typedefs; ++i) {
		if (!strcmp(typedef_tag[i], str)) {
			adv();
			tree = copy_tree(typedef_desc[i]);
			if (is_basic_type(decl_dedispatch(tree.child[0]->child[0]->head_type))) {
				id = tree.child[0]->child[0]->head_type;
				goto int_decl;
			}
			goto decl_decl2;
		}
	}

	if (valid_tree((tree = struct_decl())))
		if (tree.head_type == STRUCT_DECL)
			goto decl_decl2;
		else
			return tree;	/* named struct reference */
	if(is_basic_type(tok.type)) {
		adv();	/* eat basic-type token */
		id = decl_dispatch(tok.type);
int_decl:
		tree = new_exp_tree(id, NULL);
decl_decl2:
		/* struct-decl alone: named struct decl, e.g. struct bob { ... }; */
		if (tree.head_type == STRUCT_DECL
			&& peek().type == TOK_SEMICOLON)
			return tree;
		while (1) {
			if (!valid_tree(subtree = decl2()))
				parse_fail("bad declaration syntax");
			add_child(&tree, alloc_exptree(subtree));
			/* ',' decl2 */
			if (peek().type != TOK_COMMA)
				break;
			else
				adv();
		}
		return tree;
	}
	return null_tree;
}

/*
 * struct-decl := 'struct' [ident] '{' decl ';' { decl ';' } '}'
 *		 | 'struct' [ident] decl2 { ','  decl2 } ';'
 */
exp_tree_t struct_decl()
{
	exp_tree_t tree, subtree;
	token_t name;
	int sav_indx;
	token_t fake_anon_name = { TOK_IDENT, "<anonymous struct>", 0, 0, 0 };
	if (peek().type == TOK_STRUCT) {
		adv();
		/* [ident] */
		if ((name = peek()).type == TOK_IDENT)
			adv();
		else
			name = fake_anon_name;
		/* 'struct' [ident] '{' decl ';' { decl ';' } '}' */
		if (peek().type == TOK_LBRACE) {
			adv();
			tree = new_exp_tree(STRUCT_DECL, &name);
			while (peek().type != TOK_RBRACE) {
				subtree = decl();
				if (!valid_tree(subtree))
					parse_fail("member declaration expected in struct declaration");
				add_child(&tree, alloc_exptree(subtree));
				if (peek().type == TOK_SEMICOLON)
					adv();
				else
					break;
			}
			need(TOK_RBRACE);
			return tree;
		/* 'struct' [ident] decl2 { ','  decl2 } ';' */
		} else if (valid_tree((subtree = decl2()))) {
			tree = new_exp_tree(NAMED_STRUCT_DECL, &name);
			add_child(&tree, alloc_exptree(subtree));
			/* XXX: TODO: { , decl2 } repetition */
			return tree;
		} else
			indx = sav_indx;
	}
	return null_tree;
}

/*
 * decl2 := { '*' } ident [ { ('[' [integer] ']') } ] ['=' initializer]
 */
exp_tree_t decl2()
{
	token_t tok = peek();
	exp_tree_t tree, subtree, subtree2;
	int stars = 0;
	int i;
	token_t zero = { TOK_INTEGER, "0", 1, 0, 0 };

	tree = new_exp_tree(DECL_CHILD, NULL);

	/* 
	 * Eat pointer-qualification 
	 * stars (as in "int ***ptr") for now. 
	 * The language currently compiled is actually
	 * typeless, but I need to write the star-qualifiers
	 * in the test code so automatic comparison with
	 * gcc-compiled output is possible.
	 */
	while (peek().type == TOK_MUL)
		adv(), ++stars;
	tok = need(TOK_IDENT);
	subtree = new_exp_tree(VARIABLE, &tok);
	add_child(&tree, alloc_exptree(subtree));
	if(peek().type == TOK_LBRACK) {
multi_array_decl:
		adv();	/* eat [ */
		if (peek().type == TOK_INTEGER)
			tok = need(TOK_INTEGER);
		else
			tok = zero;
		need(TOK_RBRACK);
		add_child(&tree, 
			alloc_exptree(new_exp_tree(ARRAY_DIM, &tok)));
		if (peek().type == TOK_LBRACK)
			goto multi_array_decl;
	}
	
	/* initializer */
	if (peek().type == TOK_ASGN) {
		adv();	/* eat = */
		if (!valid_tree(subtree2 = initializer()))
			parse_fail("expected expression after =");
		add_child(&tree, alloc_exptree(subtree2));
	}

	/* add the pointer-stars */
	if (stars) {
		subtree = new_exp_tree(DECL_STAR, NULL);
		for (i = 0; i < stars; ++i)
			add_child(&tree,
				alloc_exptree(subtree));
	}

	return tree;
}

/*
 * initializer := expr
 *               | '{' {'{'} expr {'}'} [ {',' {'{'} expr {'}'}}] '}'
 */
exp_tree_t initializer()
{
	exp_tree_t tree, child;
	int depth;
	if (valid_tree((tree = expr())))
		return tree;
	/* '{' {'{'} expr {'}'} [ {',' {'{'} expr {'}'}}] '}' */
	if (peek().type == TOK_LBRACE) {
		tree = new_exp_tree(COMPLICATED_INITIALIZER, NULL);
		adv();
		while (1) {
			/* {'{'} */
			depth = 1;
			while (peek().type == TOK_LBRACE)
				++depth, adv();
			child = expr();
			if (!valid_tree(child))
				parse_fail("expression expected in array or struct initializer");
			add_child(&tree, alloc_exptree(child));
			/* {'}'} */
			while (peek().type == TOK_RBRACE)
				--depth, adv();
			if (depth == 0)
				break;
			if (peek().type != TOK_COMMA)
				break;
			else
				adv();
		}
		return tree;
	}
	return null_tree;
}

/*
 * lvalue := ident
 *          | '(' cast-type ') lvalue
 *			| '(' lvalue ')'
 */
/* XXX: TODO: cast part */
exp_tree_t lval()
{
	token_t tok = peek();
	exp_tree_t tree, subtree, new_tree;
	int sav_indx;

	/* parenthesized lvalue */
	if (peek().type == TOK_LPAREN) {
		sav_indx = adv() - 1;
		if (valid_tree((subtree = lval()))) {
			if (peek().type == TOK_RPAREN) {
				adv();
				return subtree;
			}
		}
		indx = sav_indx;
	}

	if (tok.type == TOK_IDENT) {
		/* identifier alone -- variable */
		adv();	/* eat the identifier */
		return new_exp_tree(VARIABLE, &tok);
	}
	
	return null_tree;
}

/*
	block := expr ';' 
		| decl ';'
		| if '(' expr ')' block [else block] 
		| while '(' expr ')' block 
		| for '(' [expr] ';' [expr] ';' [expr] ')' block
		| '{' { expr ';' } '}' 
		| instr '(' expr1, expr2, ..., exprN ')' ';'
		| ident ':'
		| goto ident ';'
		| [cast-type] ident '(' arg { ',' arg } ')' block
		| 'return' expr ';'
		| 'break' ';'
		| 'typedef' cast-type ident ';'
*/
exp_tree_t block()
{
	int i, args;
	exp_tree_t tree, subtree;
	exp_tree_t subtree2, subtree3;
	exp_tree_t subtree4;
	exp_tree_t proctyp, ct;
	int typed_proc = 0;
	token_t tok;
	token_t one = { TOK_INTEGER, "1", 1, 0, 0 };
	exp_tree_t one_tree = new_exp_tree(NUMBER, &one);
	int sav_indx, ident_indx, arg_indx;

	/* return-value type before a procedure */
	sav_indx = indx;
	if (valid_tree(proctyp = cast_type()))
		typed_proc = 1;

	if (peek().type != TOK_IDENT)
		indx = sav_indx;

	/* bob(a, b, c) { ... } */
	if ((tok = peek()).type == TOK_IDENT) {
		ident_indx = indx;
		if (!typed_proc)
			sav_indx = indx;
		adv();
		/*
		 * Check that it is actually a procedure
		 * definition
		 */
		if (peek().type != TOK_LPAREN)
			goto not_proc;		
		adv();
		while (peek().type != TOK_RPAREN) {
			arg_indx = indx;
			ct = cast_type();
			indx = arg_indx;
			if (valid_tree(ct))
				goto is_proc;
			if (peek().type != TOK_IDENT)
				goto not_proc;
			adv();
			if (peek().type != TOK_RPAREN)
				if (peek().type != TOK_COMMA) {
					goto not_proc;
				} else {
					adv();
				}
		}
		adv();
		if (peek().type != TOK_LBRACE)
			goto not_proc;
is_proc:
		indx = indx_bound(ident_indx + 1);			
		tree = new_exp_tree(PROC, &tok);
		/* optional return type */
		if (typed_proc)
			add_child(&tree, alloc_exptree(proctyp));
		/* argument list */
		subtree = new_exp_tree(ARG_LIST, NULL);
		need(TOK_LPAREN);
		while (peek().type != TOK_RPAREN) {
			subtree2 = arg();
			if (!valid_tree(subtree2))
				parse_fail("argument expected");
			add_child(&subtree, alloc_exptree(subtree2));
			if (peek().type != TOK_RPAREN)
				need(TOK_COMMA);
		}
		adv();
		add_child(&tree, alloc_exptree(subtree));
		subtree = block();
		if (!valid_tree(subtree))
			parse_fail("block expected");
		add_child(&tree, alloc_exptree(subtree));
		return tree;

not_proc:
		indx = sav_indx;
		/* carry on with other checks */
	}

	/* typedef cast-type ident ';' */
	if (peek().type == TOK_TYPEDEF) {
		adv();
		subtree = cast_type();
		if (!valid_tree(subtree))
			parse_fail("expected a declarator after 'typedef'");
		tok = need(TOK_IDENT);
		strcpy(typedef_tag[typedefs], get_tok_str(tok));
		typedef_desc[typedefs] = copy_tree(subtree);
		++typedefs;
		need(TOK_SEMICOLON);
		/*
		 * The left operand of the typedef must be coded
		 * itself, in case it is a struct -- structs need
		 * to be seen by the codegen so it can track struct
		 * tag names as in e.g. struct FOO { ... }; where
		 * FOO would be the tag name.
		 */
		return subtree;
	}

	/* empty block -- ';' */
	if (peek().type == TOK_SEMICOLON) {
		adv();	/* eat semicolon */
		return new_exp_tree(BLOCK, NULL);
	}
	/* label -- ident ':' */
	if (peek().type == TOK_IDENT) {
		tok = peek();
		tree = new_exp_tree(LABEL, &tok);
		adv();	/* eat label name */
		if (peek().type == TOK_COLON) {
			adv();	/* eat : */
			return tree;
		}
		/* push back the identifier, this isn't
		 * a label after all */
		else --indx;
	}
	/* decl ';' */
	if (valid_tree(tree = decl())) {
		need(TOK_SEMICOLON);
		return tree;
	}
	/* expr ';' */
	if (valid_tree(tree = expr())) {
		need(TOK_SEMICOLON);
		return tree;
	}
	/* 'break' ';' */
	if ((tok = peek()).type == TOK_BREAK) {
		adv();
		need(TOK_SEMICOLON);
		return new_exp_tree(BREAK, &tok);
	}
	/* if (expr) block1 [ else block2 ] */
	if(peek().type == TOK_IF) {
		adv();	/* eat if */
		tree = new_exp_tree(IF, NULL);
		need(TOK_LPAREN);
		add_child(&tree, alloc_exptree(expr()));
		need(TOK_RPAREN);
		add_child(&tree, alloc_exptree(block()));
		if (peek().type == TOK_ELSE) {
			adv();	/* eat else */
			add_child(&tree, alloc_exptree(block()));
		}
		return tree;
	}
	/* while (expr) block */
	if(peek().type == TOK_WHILE) {
		adv();	/* eat while */
		tree = new_exp_tree(WHILE, NULL);
		need(TOK_LPAREN);
		add_child(&tree, alloc_exptree(expr()));
		need(TOK_RPAREN);
		add_child(&tree, alloc_exptree(block()));
		return tree;
	}
	/* '{' block '}' */
	if(peek().type == TOK_LBRACE) {
		adv();	/* eat { */
		tree = new_exp_tree(BLOCK, NULL);
		while (valid_tree(subtree = block()))
			add_child(&tree, alloc_exptree(subtree));
		need(TOK_RBRACE);
		return tree;
	}
	/* instr(arg1, arg2, argN); */
	if (is_instr(peek().type)) {
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
		adv();	/* eat instruction */
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
	}
	/* 'goto' ident */
	if (peek().type == TOK_GOTO) {
		adv();	/* eat goto */
		tok = need(TOK_IDENT);
		return new_exp_tree(GOTO, &tok);
	}
	/* 'return' expr */
	if (peek().type == TOK_RET) {
		adv();	/* eat 'return' */
		tree = new_exp_tree(RET, NULL);
		subtree = expr();
		if (!valid_tree(subtree))
			parse_fail("expression expected");
		add_child(&tree, alloc_exptree(subtree));
		need(TOK_SEMICOLON);
		return tree;
	}
	/* for '(' [expr] ';' [expr] ';' [expr] ')' block */
	if (peek().type == TOK_FOR) {
		adv();	/* eat 'for' */
		need(TOK_LPAREN);
		tree = new_exp_tree(BLOCK, NULL);
		/* part 1 : initializer ... */
		if (peek().type == TOK_SEMICOLON)
			adv();	/* eat ';' -- no init */
		else {
			subtree = expr();
			if (!valid_tree(subtree))
				parse_fail("expression expected");
			add_child(&tree, alloc_exptree(subtree));
			need(TOK_SEMICOLON);
		}
		subtree = new_exp_tree(WHILE, NULL);
		/* part 2 : loop condition */
		if (peek().type == TOK_SEMICOLON) {
			/* empty conditional -- write 1 == 1 */
			adv(); /* eat ';' */
			subtree2 = new_exp_tree(EQL, NULL);
			for (i = 0; i < 2; ++i)
				add_child(&subtree2, alloc_exptree(one_tree));
		} else {
			if (!valid_tree(subtree2 = expr()))
				parse_fail("expression expected");
			need(TOK_SEMICOLON);
		}
		add_child(&subtree, alloc_exptree(subtree2));
		/* part 3: increment */
		if (peek().type == TOK_RPAREN) {
			subtree3 = null_tree;
			adv();	/* no increment, eat ')' */
		} else {
			if (!valid_tree(subtree3 = expr()))
				parse_fail("expression expected");
			need(TOK_RPAREN);
		}
		subtree4 = new_exp_tree(BLOCK, NULL);
		if (!valid_tree(subtree2 = block()))
			parse_fail("block expected");
		add_child(&subtree4, alloc_exptree(subtree2));
		/* add increment (if any) to end of block */
		if (valid_tree(subtree3))
			add_child(&subtree4, alloc_exptree(subtree3));
		add_child(&subtree, alloc_exptree(subtree4));
		add_child(&tree, alloc_exptree(subtree));
		return tree;
	}

	return null_tree;
}

/*
	arg := [cast-type] ident
*/
exp_tree_t arg()
{
	exp_tree_t ct;
	exp_tree_t at = new_exp_tree(ARG, NULL);
	token_t tok;

	if (valid_tree((ct = cast_type())))
		add_child(&at, alloc_exptree(ct));
	tok = need(TOK_IDENT);
	add_child(&at, alloc_exptree(
		new_exp_tree(VARIABLE, &tok)));

	return at;
}

/*
	expr := e1 asg-op expr 
		| ternary_expr
		| 'int' ident [ ( '=' expr ) |  ('[' integer ']') ]
		| str-const
*/
exp_tree_t expr()
{
	exp_tree_t tree, subtree, subtree2;
	exp_tree_t subtree3;
	exp_tree_t lv;
	token_t oper;
	token_t tok;
	int save = indx;

	if (valid_tree(lv = e1())) {
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
				case TOK_MODEQ:
					subtree = new_exp_tree(MOD, NULL);
					break;
				case TOK_ASGN:
					break;
				default:
					parse_fail
					 ("invalid assignment operator");
			}
			adv() - 1;	/* eat asg-op */
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
	/* ternary_expr */
	if (valid_tree(tree = ternary_expr())) {
		return tree;
	}
	/* "bob123" */
	if ((tok = peek()).type == TOK_STR_CONST) {
		adv();
		tree = new_exp_tree(STR_CONST, &tok);
		return tree;
	}

	return null_tree;
}

/* 
 * General parsing routine for 
 * left-associative operators:
 * A-expr := B-expr { C-op B-expr } 
 *
 * if no_assoc is set, it instead parses
 * the non-associative but similary
 * structured form:
 * A-expr := B-expr [ C-op B-expr ]
 * 
 * anonymous functions AKA lambdas
 * would have been nice to have in C
 */
exp_tree_t parse_left_assoc(
	exp_tree_t(*B_expr)(),
	int(*is_C_op)(char),
	void(*tree_dispatch)(char, exp_tree_t*),
	int no_assoc)
{
	exp_tree_t child, tree, new;
	exp_tree_t *child_ptr, *tree_ptr, *new_ptr;
	exp_tree_t *root;
	int prev;
	int cc = 0;
	token_t oper;

	if (!valid_tree(child = B_expr()))
		return null_tree;
	
	if (!is_C_op((oper = peek()).type))
		return child;
	
	child_ptr = alloc_exptree(child);
	
	tree_dispatch(oper.type, &tree);

	prev = oper.type;
	adv();	/* eat C-op */
	tree_ptr = alloc_exptree(tree);
	add_child(tree_ptr, child_ptr);

	while (1) {
		if (!valid_tree(child = B_expr()))
			parse_fail("expression expected");

		/* add term as child */
		add_child(tree_ptr, alloc_exptree(child));

		/* non-associativity corner-case,
		 * used for comparison operators */
		if (cc++ && no_assoc)
			parse_fail("illegal use of non-associative operator");

		/* bail out early if no more operators */
		if (!is_C_op((oper = peek()).type))
			return *tree_ptr;

		tree_dispatch(oper.type, &new);

		adv();	/* eat C-op */

		new_ptr = alloc_exptree(new);
		add_child(new_ptr, tree_ptr);
		tree_ptr = new_ptr;
	}

	return *tree_ptr;
}

/*
	ternary-expr := ccor_expr 
	                | expr '?' ccor_expr ':' expr
*/
exp_tree_t ternary_expr()
{
	exp_tree_t tree0, tree1, tree2, full;
	tree0 = ccor_expr();
	if (!valid_tree(tree0))
		return null_tree;
	if (peek().type ==  TOK_QMARK) {
		adv();
		full = new_exp_tree(TERNARY, NULL);
		if (!valid_tree(tree1 = expr()))
			parse_fail("expression expected after ternary `?'");
		need(TOK_COLON);
		if (!valid_tree(tree2 = expr()))
			parse_fail("expression expected after ternary `:'");
		add_child(&full, alloc_exptree(tree0));
		add_child(&full, alloc_exptree(tree1));
		add_child(&full, alloc_exptree(tree2));
		return full;
	} else
		return tree0;
}

/* ccor_expr := ccand_expr ['||' ccand_expr] */
void ccor_dispatch(char oper, exp_tree_t *dest)
{
	switch (oper) {
		case TOK_CC_OR:
			*dest = new_exp_tree(CC_OR, NULL);
		break;
	}
}
int is_ccor_op(char ty) {
	return ty == TOK_CC_OR;
}
exp_tree_t ccor_expr()
{
	return parse_left_assoc(
		&ccand_expr,
		&is_ccor_op,
		&ccor_dispatch,
		0);
}

/* ccand_expr := comp_expr ['&&' comp_expr] */
void ccand_dispatch(char oper, exp_tree_t *dest)
{
	switch (oper) {
		case TOK_CC_AND:
			*dest = new_exp_tree(CC_AND, NULL);
		break;
	}
}
int is_ccand_op(char ty) {
	return ty == TOK_CC_AND;
}
exp_tree_t ccand_expr()
{
	return parse_left_assoc(
		&comp_expr,
		&is_ccand_op,
		&ccand_dispatch,
		0);
}

/* comp_expr := sum_expr [comp-op sum_expr] */
void comp_dispatch(char oper, exp_tree_t *dest)
{
	switch (oper) {
		case TOK_GT:
			*dest = new_exp_tree(GT, NULL);
			break;
		case TOK_GTE:
			*dest = new_exp_tree(GTE, NULL);
			break;
		case TOK_LT:
			*dest = new_exp_tree(LT, NULL);
			break;
		case TOK_LTE:
			*dest = new_exp_tree(LTE, NULL);
			break;
		case TOK_EQ:
			*dest = new_exp_tree(EQL, NULL);
			break;
		case TOK_NEQ:
			*dest = new_exp_tree(NEQL, NULL);
			break;
	}
}
exp_tree_t comp_expr()
{
	return parse_left_assoc(
		&sum_expr,
		&is_comp_op,
		&comp_dispatch,
		1);
}

/* sum_expr := mul_expr { add-op mul_expr } */
void sum_dispatch(char oper, exp_tree_t *dest)
{
	switch (oper) {
		case TOK_PLUS:
			*dest = new_exp_tree(ADD, NULL);
		break;
		case TOK_MINUS:
			*dest = new_exp_tree(SUB, NULL);
		break;
	}
}
exp_tree_t sum_expr()
{
	return parse_left_assoc(
		&mul_expr,
		&is_add_op,
		&sum_dispatch,
		0);
}

/* mul_expr := e1 { mul-op e1 } */
void mul_dispatch(char oper, exp_tree_t *dest)
{
	switch (oper) {
		case TOK_DIV:
			*dest = new_exp_tree(DIV, NULL);
		break;
		case TOK_MUL:
			*dest = new_exp_tree(MULT, NULL);
		break;
		case TOK_MOD:
			*dest = new_exp_tree(MOD, NULL);
		break;
	}
}
exp_tree_t mul_expr()
{
	return parse_left_assoc(
		&e1,
		&is_mul_op,
		&mul_dispatch,
		0);
}

/*
	e1 := '++' e1
		 | '--' e1
		 | '-' e1
		 | '!' e1
		 | cast
		 | '*' e1
		 | '&' e1
		 | e0
*/
exp_tree_t e1()
{
	exp_tree_t tree, subtree;
	token_t tok = peek();

	/* ++ e1 */
	if (tok.type == TOK_PLUSPLUS) {
		adv(); /* eat ++ */
		tree = new_exp_tree(INC, NULL);
		goto e1_prefix;
	}
	/* -- e1 */
	if (tok.type == TOK_MINUSMINUS) {
		adv(); /* eat -- */
		tree = new_exp_tree(DEC, NULL);
		goto e1_prefix;
	}
	/* - e1 */
	if (tok.type == TOK_MINUS) {
		adv(); /* eat - */
		tree = new_exp_tree(NEGATIVE, NULL);
		goto e1_prefix;
	}
	/* ! e1 */
	if (tok.type == TOK_CC_NOT) {
		adv(); /* eat ! */
		tree = new_exp_tree(CC_NOT, NULL);
		goto e1_prefix;
	}
	/* cast */
	if (valid_tree(tree = cast()))
		return tree;
	/* * e1 */
	if (tok.type == TOK_MUL) {
		adv();
		tree = new_exp_tree(DEREF, NULL);
		goto e1_prefix;
	}
	/* & e1 */
	if (tok.type == TOK_ADDR) {
		adv();
		tree = new_exp_tree(ADDR, NULL);
		goto e1_prefix;
	}

	/* try the higher-precedence stuff on its own */
	return e0();

e1_prefix:
	subtree = e1();
	if (!valid_tree(subtree))
		parse_fail("expression expected after prefix operator");
	add_child(&tree,
		alloc_exptree(subtree));
	return tree;
}

/*
	e0 :=
		| e0_2 '++'
		| e0_2 '--'
		| e0_2
*/
exp_tree_t e0()
{
	exp_tree_t tree0, tree1, tree2, new_tree;

	tree0 = e0_2();
	if (!valid_tree(tree0))
		return null_tree;

	/* post ++ */
	if (peek().type == TOK_PLUSPLUS) {
		adv();
		tree1 = new_exp_tree(POST_INC, NULL);
		add_child(&tree1, alloc_exptree(tree0));
		return tree1;
	}

	/* post -- */
	if (peek().type == TOK_MINUSMINUS) {
		adv();
		tree1 = new_exp_tree(POST_DEC, NULL);
		add_child(&tree1, alloc_exptree(tree0));
		return tree1;
	}

	/* e0_2 as-is */
	return tree0;
}

/*
	e0_2 := e0_3 [ { '[' expr ']' } [('->' | '.') e0_3]
*/
exp_tree_t e0_2()
{
	exp_tree_t tree0, tree1, tree2, addr, new_tree;

	tree0 = e0_3();
	if (!valid_tree(tree0))
		return null_tree;

eval_e02:

	/* [ ] -- array access */
	if(peek().type == TOK_LBRACK) {
		tree1 = new_exp_tree(ARRAY, NULL);
		add_child(&tree1, alloc_exptree(tree0));
		adv() - 1;	/* eat [ */
multi_array_access:
		if (!valid_tree(tree2 = expr()))
			parse_fail("expression expected");
		add_child(&tree1, alloc_exptree(tree2));
		need(TOK_RBRACK);
		if (peek().type == TOK_LBRACK) {
			adv();
			/*
		 	 * Make a nested ARRAY node so that e.g. mat[i][j] parses to:
			 * (ARRAY (ARRAY (VARIABLE:matrix) (VARIABLE:i)) (VARIABLE:j))
			 * 
			 * The motivation of this design is that it allows the
			 * codegen to write code for N-dimension arrays just as if
			 * they were all 1-dimensional arrays by recursing the
			 * code generation of the base pointer. (Well TBH I have
			 * only tested it with 2D at this point).
			 */
			new_tree = new_exp_tree(ARRAY, NULL);
			add_child(&new_tree, alloc_exptree(tree1));
			tree1 = new_tree;
			goto multi_array_access;
		}

		/* a[b]->c */
		if (peek().type == TOK_ARROW) {
			new_tree = new_exp_tree(DEREF_STRUCT_MEMB, NULL);
			add_child(&new_tree, alloc_exptree(tree1));
			adv();
			tree2 = e0_3();
			if (!valid_tree(tree2))
				parse_fail("expected unary expression after ->");
			add_child(&new_tree, alloc_exptree(tree2));
			/* a[b]->c[d] ??!!! */
			if (peek().type == TOK_LBRACK) {
				tree0 = new_tree;
				goto eval_e02;
			}
			return new_tree;
		}

		/* a[b].c */
		if (peek().type == TOK_DOT) {
			new_tree = new_exp_tree(DEREF_STRUCT_MEMB, NULL);
			addr = new_exp_tree(ADDR, NULL);
			add_child(&addr, alloc_exptree(tree1));
			add_child(&new_tree, alloc_exptree(addr));
			adv();
			tree2 = e0_3();
			if (!valid_tree(tree2))
				parse_fail("expected unary expression after .");
			add_child(&new_tree, alloc_exptree(tree2));
			/* a[b].c[d] ??!!! */
			if (peek().type == TOK_LBRACK) {
				tree0 = new_tree;
				goto eval_e02;
			}
			return new_tree;
		}

		return tree1;
	}

	/* e0_3 as-is */
	return tree0;
}

/*
	e0_3 :=
		 e0_4 {'.' e0_4}
		| e0_4 {'->' e0_4}
		| e0_4
*/
void e0_3_dispatch(char oper, exp_tree_t *dest)
{
	switch (oper) {
		case TOK_DOT:
			*dest = new_exp_tree(STRUCT_MEMB, NULL);
		break;
		case TOK_ARROW:
			*dest = new_exp_tree(DEREF_STRUCT_MEMB, NULL);
		break;
	}
}
int check_e0_3_op(char oper)
{
	return oper == TOK_DOT || oper == TOK_ARROW;
}
/*
 * rewrite a.b as (&a)->b
 *
 * this was inspired by reading the sources
 * for the V7 UNIX C compiler for fun
 * (http://minnie.tuhs.org/Archive/PDP-11/Distributions/research/Henry_Spencer_v7/v7.tar.gz)
 *
 * v7/usr/src/cmd/c/c01.c, line 113 does exactly
 * this transformation, but in only 8 lines of code !
 */
void e0_3_rewrite(exp_tree_t *tree)
{
	exp_tree_t fake_tree, fake_tree_2;
	int i;
	if (tree->head_type == STRUCT_MEMB) {
		fake_tree = new_exp_tree(ADDR, NULL);
		add_child(&fake_tree, tree->child[0]);
		fake_tree_2 = new_exp_tree(DEREF_STRUCT_MEMB, NULL);
		add_child(&fake_tree_2, alloc_exptree(fake_tree));
		add_child(&fake_tree_2, tree->child[1]);
		memcpy(tree, &fake_tree_2, sizeof(exp_tree_t));
	}
	for (i = 0; i < tree->child_count; ++i)
		e0_3_rewrite(tree->child[i]);
}
exp_tree_t e0_3()
{
	exp_tree_t et = parse_left_assoc(
		&e0_4,
		&check_e0_3_op,
		&e0_3_dispatch,
		0);
	e0_3_rewrite(&et);
	return et;
}

/*
	e0_4 := 
		'sizeof' '(' cast-type ')'
		| 'sizeof' expr
		| ident '(' expr1, expr2, exprN ')'
		| '(' expr ')'
		| lvalue
		| integer
		| octal-integer
		| hex-integer
		| char-const
*/
exp_tree_t e0_4()
{
	exp_tree_t tree = null_tree, subtree = null_tree;
	exp_tree_t subtree2 = null_tree, subtree3;
	token_t tok = peek();
	token_t fake_int;
	char *buff;
	int val;
	int neg = 0;
	int paren;

	/*
	 * sizeof expr
	 */
	if (peek().type == TOK_SIZEOF) {
		/* 
		 * Try cast-type before expression,
		 * because otherwise sizeof(foo_t) where foo_t
		 * is a typedef tag may be misinterpreted
		 * as sizeof an non-existent variable "foo_t"
		 */
		adv();
		tree = new_exp_tree(SIZEOF, NULL);
		if (peek().type == TOK_LPAREN) {
			adv();
			paren = 1;
		} else paren = 0;
		if (valid_tree((subtree = cast_type())))
			add_child(&tree, alloc_exptree(subtree));
		else if (valid_tree((subtree = expr())))
			add_child(&tree, alloc_exptree(subtree));
		if (paren)
			need(TOK_RPAREN);
		return tree;
	}

	/* 
	 * ident ( expr1, expr2, exprN )
	 * (function call)
	 */
	if (peek().type == TOK_IDENT) {
		tok = peek();
		adv();
		if (peek().type == TOK_LPAREN) {
			adv();	/* eat ( */
			tree = new_exp_tree(PROC_CALL, &tok);
			while (peek().type != TOK_RPAREN) {
				subtree2 = expr();
				if (!valid_tree(subtree2))
					parse_fail("expression expected");
				add_child(&tree, alloc_exptree(subtree2));
				if (peek().type != TOK_RPAREN)
					need(TOK_COMMA);
			}
			adv();
			return tree;
		} else
			--indx;
	}

	/* 
	 * Parenthesized expression
	 */
	if(peek().type == TOK_LPAREN) {
		adv();	/* eat ( */
		tree = expr();
		need(TOK_RPAREN);
		return tree;
	}

	/*
	 * lvalue
	 */
	if (valid_tree((tree = lval())))
		return tree;

	/*
	 * Integer and character constants
	 */
	if (tok.type == TOK_OCTAL_INTEGER) {
		fake_int.type = TOK_INTEGER;
		buff = malloc(256);
		strncpy(buff, tok.start + 1, tok.len - 1);
		sscanf(buff, "%o", &val);
		sprintf(buff, "%d", val);
		fake_int.start = buff;
		fake_int.len = strlen(buff);
		fake_int.from_line = 0;
		fake_int.from_line = 1;
		tree = new_exp_tree(NUMBER, &fake_int);
		adv();	/* eat number */
		return tree;
	}
	if (tok.type == TOK_HEX_INTEGER) {
		fake_int.type = TOK_INTEGER;
		buff = malloc(256);
		strncpy(buff, tok.start + 2, tok.len - 2);
		sscanf(buff, "%x", &val);
		sprintf(buff, "%d", val);
		fake_int.start = buff;
		fake_int.len = strlen(buff);
		fake_int.from_line = 0;
		fake_int.from_line = 1;
		tree = new_exp_tree(NUMBER, &fake_int);
		adv();	/* eat number */
		return tree;
	}
	if (tok.type == TOK_CHAR_CONST) {
		fake_int.type = TOK_INTEGER;
		buff = malloc(8);
		sprintf(buff, "%d", *(tok.start + 1));
		fake_int.start = buff;
		fake_int.len = strlen(buff);
		fake_int.from_line = 0;
		fake_int.from_line = 1;
		tree = new_exp_tree(NUMBER, &fake_int);
		adv();	/* eat number */
		return tree;
	}
	if (tok.type == TOK_CHAR_CONST_ESC) {
		fake_int.type = TOK_INTEGER;
		buff = malloc(8);
		sprintf(buff, "%d", escape_code(*(tok.start + 2)));
		fake_int.start = buff;
		fake_int.len = strlen(buff);
		fake_int.from_line = 0;
		fake_int.from_line = 1;
		tree = new_exp_tree(NUMBER, &fake_int);
		adv();	/* eat number */
		return tree;
	}
	if (tok.type == TOK_INTEGER) {
		tree = new_exp_tree(NUMBER, &tok);
		adv();	/* eat number */
		return tree;
	}

	return null_tree;
}

