#include "tree.h"
#include "tokens.h"
#include "codegen.h"
#include <string.h>
#include <stdio.h>

extern void fail(char*);

/* tree -> code generator */
/* TODO: - implement ==
 *	 - eliminate repetitions
 */

int temp_register = 245;

char symtab[256][32] = {""};
int syms = 0;

int get_temp_storage() {
	if (temp_register > 255) {
		fail("expression stack overflow");
	}
	return temp_register++;
}

void new_temp_storage() {
	temp_register = 245;
}

char **code_text;
int code_toks = 0;
int code_toks_alloc = 0;

void print_code()
{
	int i;
	for (i = 0; i < code_toks; i++) {
		printf("%s", code_text[i]);
		if (code_text[i][strlen(code_text[i]) - 1] != '\n')
			printf(" ");
	}
	printf("\n"); 
}

void push_line(char *lin)
{
	extern char* push_compiled_token(char *tok);
	char buf[1024];
	char *p;
	strcpy(buf, lin);
	p = strtok(buf, " ");
	do {
		push_compiled_token(p);
	} while((p = strtok(NULL, " ")));
}

char* push_compiled_token(char *tok)
{
	int i;
	if (++code_toks > code_toks_alloc) {
		code_toks_alloc += 64;
		code_text = realloc(code_text,
			code_toks_alloc * sizeof(char *));
		for (i = code_toks_alloc - 64;
			i < code_toks_alloc; i++)
			if (!(code_text[i] = malloc(64)))
				fail("alloc text token");
	
		if (!code_text)
			fail("realloc codegen tokens");
	}
	strcpy(code_text[code_toks - 1], tok);
	return code_text[code_toks - 1];
}

char* get_tok_str(token_t t)
{
	static char buf[1024];
	strncpy(buf, t.start, t.len);
	buf[t.len] = 0;
	return buf;
}

int sym_check(char* s)
{
	int i;
	for (i = 0; i < 256; i++)
		if (!strcmp(symtab[i], s))
			return 1;
	return 0;
}

int nameless_perm_storage()
{
	if (syms >= 245)
		fail("out of permanent registers");
	return syms++;
}

int sym_add(char *s)
{
	strcpy(symtab[syms], s);
	if (syms >= 245)
		fail("out of permanent registers");
	return syms++; 
}

int sym_lookup(char* s)
{
	char buf[1024];
	int i =0;
	for (i = 0; i < syms; i++)
		if (!strcmp(symtab[i], s))
			return i;
	sprintf(buf, "unknown symbol `%s'", s);
	fail(buf);
}

int arith_op(int ty)
{
	switch (ty) {
		case ADD:
			return 20;
		case SUB:
			return 30;
		case MULT:
			return 40;
		case DIV:
			return 50;
		default:
			return 0;
	}
}

codegen_t codegen(exp_tree_t* tree)
{
	int sto;
	int i;
	int sym;
	int oper;
	int arith;
	char *name;
	int bytesize = 0;
	codegen_t cod;
	char buf[1024];
	char *bp1, *bp2;
	int while_start;
	token_t one = { TOK_INTEGER, "1", 1, 0, 0 };
	exp_tree_t one_tree = new_exp_tree(NUMBER, &one);
	exp_tree_t *new, *new2;
	exp_tree_t *temp;

	/* 
	 * BPF integer comparisons / bool system...
	 * This system considers an expression "true" whenever
	 * it is <= 0. This is because we use a "zbPtrTo var1, 0, var2"
	 * instruction for conditional branching. 
	 */

	/* Rewrite a <= b as a - b */
	if (tree->head_type == LTE) {
		tree->head_type = SUB;
	}

	/* Rewrite a < b as a - b + 1 */
	if (tree->head_type == LT) {
		new = alloc_exptree(new_exp_tree(ADD, NULL));
		new2 = alloc_exptree(new_exp_tree(SUB, NULL));
		add_child(new2, tree->child[0]);
		add_child(new2, tree->child[1]);
		add_child(new, new2);
		add_child(new, alloc_exptree(one_tree));
		*tree = *new;
	}

	/* Rewrite a >= b as b - a */
	if (tree->head_type == GTE) {
		temp = tree->child[0];
		tree->child[0] = tree->child[1];
		tree->child[1] = temp;
		tree->head_type = SUB;
	}

	/* Rewrite a > b as b - a + 1 */
	if (tree->head_type == GT) {
		new = alloc_exptree(new_exp_tree(ADD, NULL));
		new2 = alloc_exptree(new_exp_tree(SUB, NULL));
		add_child(new2, tree->child[1]);
		add_child(new2, tree->child[0]);
		add_child(new, new2);
		add_child(new, alloc_exptree(one_tree));
		*tree = *new;
	}

	if (tree->head_type == BLOCK
		|| tree->head_type == IF
		|| tree->head_type == WHILE
		|| tree->head_type == ASGN) {
		/* clear expression stack */
		new_temp_storage();
	}
	
	/* block */
	if (tree->head_type == BLOCK) {
		/* codegen expressions in block */
		for (i = 0; i < tree->child_count; i++)
			bytesize += codegen(tree->child[i]).bytes;
		return (codegen_t){ 0, bytesize };
	}

	/* variable declaration, with optional assignment */
	if (tree->head_type == INT_DECL) {
		name = get_tok_str(*(tree->child[0]->tok));
		if (!sym_check(name))
			sym = sym_add(name);
		if (tree->child_count == 2) {
			if (tree->child[1]->head_type == NUMBER) {
				sprintf(buf, "Do %d 10 1 %s\n", sym,
					get_tok_str(*(tree->child[1]->tok)));
				push_line(buf);
				return (codegen_t){ 0, 5 };
			} else if (tree->child[1]->head_type == VARIABLE) {
				sto = sym_lookup(get_tok_str(*(tree->child[1]->tok)));
				sprintf(buf, "Do %d 10 2 %d\n", sym, sto);
				push_line(buf);
				return (codegen_t){ 0, 5 };
			} else {
				cod = codegen(tree->child[1]);
				sprintf(buf, "Do %d 10 2 %d\n", sym, cod.adr);
				push_line(buf);
				return (codegen_t){ 0, cod.bytes + 5 };
			}
		}
		return (codegen_t){ 0, 0 };
	}

	/* direct instruction */
	if (tree->head_type == BPF_INSTR) {
		name = get_tok_str(*(tree->tok));
		if (!strcmp(name, "echo")) {
			if(tree->child[0]->head_type == VARIABLE) {
				sym = sym_lookup(get_tok_str(*(tree->child[0]->tok)));
				sprintf(buf, "Echo %d\n", sym);
				push_line(buf);
				return (codegen_t){ 0, 2 };
			} else {
				cod = codegen(tree->child[0]);
				sprintf(buf, "Echo %d\n", cod.adr);
				push_line(buf);
				return (codegen_t){ 0, cod.bytes + 2 };
			}
		} else
			fail("can't compile that instruction yet");
		/* TODO: more instructions. Won't be simple
		 * because some parameters have to be addresses,
		 * some have to be numbers.
		 */
	}

	/* pre-increment, pre-decrement */
	if (tree->head_type == INC
		|| tree->head_type == DEC) {
		name = get_tok_str(*(tree->child[0]->tok));
		sym = sym_lookup(name);
		sto = get_temp_storage();
		sprintf(buf, "Do %d %d 1 1\n", sym,
			tree->head_type == INC ? 20 : 30);
		push_line(buf);
		sprintf(buf, "Do %d 10 2 %d\n", sto, sym);
		push_line(buf);
		return (codegen_t) { sto, 10 };
	}

	/* post-increment, post-decrement */
	if (tree->head_type == POST_INC
		|| tree->head_type == POST_DEC) {
		name = get_tok_str(*(tree->child[0]->tok));
		sym = sym_lookup(name);
		sto = get_temp_storage();
		sprintf(buf, "Do %d 10 2 %d\n", sto, sym);
		push_line(buf);
		sprintf(buf, "Do %d %d 1 1\n", sym,
			tree->head_type == POST_INC ? 20 : 30);
		push_line(buf);
		return (codegen_t) { sto, 10 };
	}

	/* assignment */
	if (tree->head_type == ASGN && tree->child_count == 2) {
		sym = sym_lookup(get_tok_str(*(tree->child[0]->tok)));
		if (tree->child[1]->head_type == NUMBER) {
			sprintf(buf, "Do %d 10 1 %s\n", sym,
				get_tok_str(*(tree->child[1]->tok)));
			push_line(buf);
			return (codegen_t){ sym, 5 };
		} else if (tree->child[1]->head_type == VARIABLE) {
			sto = sym_lookup(get_tok_str(*(tree->child[i]->tok)));
			sprintf(buf, "Do %d 10 2 %d\n", sym, sto);
			push_line(buf);
			return (codegen_t){ sym, 5 };
		} else {
			cod = codegen(tree->child[1]);
			sprintf(buf, "Do %d 10 2 %d\n", sym, cod.adr);
			push_line(buf);
			return (codegen_t){ sym, cod.bytes + 5 };
		}
	}

	/* if */
	if (tree->head_type == IF) {
		/* the conditional */
		cod = codegen(tree->child[0]);
		sto = get_temp_storage();
		sprintf(buf, "Do %d 10 1 ", sto);
		push_line(buf);
		bp1 = push_compiled_token("_");
		push_compiled_token("\n");
		sprintf(buf, "zbPtrTo %d 0 %d\n", cod.adr, sto);
		push_line(buf);
		bytesize += codegen(tree->child[1]).bytes;
		if (tree->child_count == 3) {
			bytesize += 9;
		}
		/* backpatch 1 */
		sprintf(buf, "%d", bytesize);
		strcpy(bp1, buf);

		bytesize += 9 + cod.bytes;

		/* else ? */
		if (tree->child_count == 3) {
			sto = get_temp_storage();
			sprintf(buf, "PtrTo %d\n", sto);
			push_line(buf);
			sprintf(buf, "Do %d 20 1 ", sto);
			push_line(buf);
			bp2 = push_compiled_token("_");
			push_compiled_token("\n");
			sprintf(buf, "PtrFrom %d\n", sto);
			push_line(buf);
			cod = codegen(tree->child[2]);
			/* backpatch 2 */
			sprintf(buf, "%d", 7 + cod.bytes);
			strcpy(bp2, buf);
			bytesize += cod.bytes;
		}

		return (codegen_t){ 0, bytesize };
	}

	/* while */
	if (tree->head_type == WHILE) {
		/* use permanent storage for the
		 * while conditional jump pointer */
		while_start = nameless_perm_storage();
		sprintf(buf, "PtrTo %d\n", while_start);
		push_line(buf);
		/* the conditional */
		cod = codegen(tree->child[0]);
		sto = get_temp_storage();
		sprintf(buf, "Do %d 10 1 ", sto);
		push_line(buf);
		bp1 = push_compiled_token("_");
		push_compiled_token("\n");
		sprintf(buf, "zbPtrTo %d 0 %d\n", cod.adr, sto);
		push_line(buf);
		bytesize += codegen(tree->child[1]).bytes;
		/* jump back to before the conditional */
		sprintf(buf, "PtrFrom %d\n", while_start);
		push_line(buf);
		bytesize += 2;
		/* backpatch 1 */
		sprintf(buf, "%d", bytesize);
		strcpy(bp1, buf);

		bytesize += 9 + cod.bytes;

		return (codegen_t){ 0, bytesize };
	}

	/* negative sign */
	if (tree->head_type == NEGATIVE) {
		sto = get_temp_storage();
		cod = codegen(tree->child[0]);
		sprintf(buf, "Do %d 10 1 0\n", sto);
		push_line(buf);
		sprintf(buf, "Do %d 30 2 %d\n", sto, cod.adr);
		push_line(buf);
		return (codegen_t) {sto, 10 + cod.bytes };
	}

	/* number */
	if (tree->head_type == NUMBER) {
		sto = get_temp_storage();
		sprintf(buf, "Do %d 10 1 %s\n", 
			sto,
			get_tok_str(*(tree->tok)));
		push_line(buf);
		return (codegen_t) { sto, 5 };
	}

	/* variable */
	if (tree->head_type == VARIABLE) {
		sto = get_temp_storage();
		sym = sym_lookup(get_tok_str(*(tree->tok)));
		sprintf(buf, "Do %d 10 2 %d\n", 
			sto,
			sym);
		push_line(buf);
		return (codegen_t) { sto, 5 };
	}

	/* arithmetic */
	if ((arith = arith_op(tree->head_type)) && tree->child_count) {
		sto = get_temp_storage();
		for (i = 0; i < tree->child_count; i++) {
			oper = i ? arith : 10;
			if (tree->child[i]->head_type == NUMBER) {
				sprintf(buf, "Do %d %d 1 %s\n", sto, oper,
					get_tok_str(*(tree->child[i]->tok)));
				push_line(buf);
				bytesize += 5;
			} else if(tree->child[i]->head_type == VARIABLE) {
				sym = sym_lookup(get_tok_str(*(tree->child[i]->tok)));
				sprintf(buf, "Do %d %d 2 %d\n", sto, oper, sym);
				push_line(buf);
				bytesize += 5;
			} else {
				cod = codegen(tree->child[i]);
				sprintf(buf, "Do %d %d 2 %d\n", sto, oper, cod.adr);
				push_line(buf);
				bytesize += 5 + cod.bytes;
			}
		}
		return (codegen_t){ sto, bytesize };
	}

	printf("Sorry, I can't codegen %s yet\n",
		tree_nam[tree->head_type]);
	exit(1);
}
