#include "tree.h"
#include "codegen.h"
#include <string.h>
#include <stdio.h>

extern void fail(char*);

/* tree -> code generator */
/* TODO: - implement all the trees
 *		  - always give compiled code byte size
 *			(needed to implement if/if-else/while codegen)
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

int sym_add(char *s)
{
	strcpy(symtab[syms], s);
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
	codegen_t cod;

	/* block */
	if (tree->head_type == BLOCK) {
		/* codegen expressions in block */
		for (i = 0; i < tree->child_count; i++)
			codegen(tree->child[i]);
		/* TODO: return size of code inside */
		return (codegen_t){ 0, 0 };
	}

	/* variable declaration, with optional assignment */
	if (tree->head_type == INT_DECL) {
		new_temp_storage();
		name = get_tok_str(*(tree->child[0]->tok));
		if (!sym_check(name))
			sym = sym_add(name);
		if (tree->child_count == 2) {
			sto = codegen(tree->child[1]).adr;
			printf("Do %d 10 2 %d\n", sym, sto);
			return (codegen_t){ sto, 0 };
		}
		return (codegen_t){ 0, 0 };
	}

	/* direct instruction */
	if (tree->head_type == BPF_INSTR) {
		name = get_tok_str(*(tree->tok));
		if (!strcmp(name, "echo")) {
			if(tree->child[0]->head_type == VARIABLE) {
				sym = sym_lookup(get_tok_str(*(tree->child[0]->tok)));
				printf("Echo %d\n", sym);
			} else {
				cod = codegen(tree->child[0]);
				printf("Echo %d\n", cod.adr);
			}
		} else
			fail("can't compile that instruction yet");
		/* TODO: more instructions. Won't be simple
		 * because some parameters have to be addresses,
		 * some have to be numbers.
		 */

		return (codegen_t){ 0, 0 };
	}

	/* pre-increment, pre-decrement */
	if (tree->head_type == INC
		|| tree->head_type == DEC) {
		name = get_tok_str(*(tree->child[0]->tok));
		sym = sym_lookup(name);
		sto = get_temp_storage();
		printf("Do %d %d 1 1\n", sym,
			tree->head_type == INC ? 20 : 30);
		printf("Do %d 10 2 %d\n", sto, sym);
		return (codegen_t) { sto, 0 };
	}

	/* post-increment, post-decrement */
	if (tree->head_type == POST_INC
		|| tree->head_type == POST_DEC) {
		name = get_tok_str(*(tree->child[0]->tok));
		sym = sym_lookup(name);
		sto = get_temp_storage();
		printf("Do %d 10 2 %d\n", sto, sym);
		printf("Do %d %d 1 1\n", sym,
			tree->head_type == POST_INC ? 20 : 30);
		return (codegen_t) { sto, 0 };
	}

	/* assignment */
	if (tree->head_type == ASGN && tree->child_count == 2) {
		new_temp_storage();
		sto = codegen(tree->child[1]).adr;
		sym = sym_lookup(get_tok_str(*(tree->child[0]->tok)));
		printf("Do %d 10 2 %d\n", sym, sto);
		return (codegen_t) { sto, 0 };
	}

	/* number */
	if (tree->head_type == NUMBER) {
		sto = get_temp_storage();
		printf("Do %d 10 1 %s\n", 
			sto,
			get_tok_str(*(tree->tok)));
		return (codegen_t) { sto, 0 };
	}

	/* arithmetic */
	if ((arith = arith_op(tree->head_type)) && tree->child_count) {
		sto = get_temp_storage();
		for (i = 0; i < tree->child_count; i++) {
			oper = i ? arith : 10;
			if (tree->child[i]->head_type == NUMBER) {
				printf("Do %d %d 1 %s\n", sto, oper,
					get_tok_str(*(tree->child[i]->tok)));
			} else if(tree->child[i]->head_type == VARIABLE) {
				sym = sym_lookup(get_tok_str(*(tree->child[i]->tok)));
				printf("Do %d %d 2 %d\n", sto, oper, sym);
			} else {
				cod = codegen(tree->child[i]);
				printf("Do %d %d 2 %d\n", sto, oper, cod.adr);
			}
		}
		return (codegen_t){ sto, 0 };
	}

	printf("Sorry, I can't codegen %s yet\n",
		tree_nam[tree->head_type]);
	exit(1);
}
