/* Syntax tree -> BPF VM bytecode generator */

/* TODO: - eliminate repetitions, make routines for them (DRY)
 *        - try porting this to a real architecture one day ? 
 */

#define EXPR_STACK_SIZE 32

#include "tree.h"
#include "tokens.h"
#include "codegen_bpf.h"
#include <string.h>
#include <stdio.h>

int temp_register = 255 - EXPR_STACK_SIZE;
char symtab[256][32] = {""};
char *instr_arg[32];
int syms = 1;
int label_count = 0;
char *label_bp[256];	/* label backpatches */
char *label_bp_2[256];
int byte_count = 0;
int program_ptr;
char **code_text;
int code_toks = 0;
int code_toks_alloc = 0;

extern void fail(char*);
extern void compiler_fail(char *message, token_t *token,
	int in_line, int in_chr);

/* number = mult * 255 + mod 
 * range of possible values: 0 to 65280 */
typedef struct encoded_int {
	int mult;
	int mod;
} encoded_int_t;

encoded_int_t encode(int n) {
	int m = 0;
	while (n > 255) {
		n -= 255;
		++m;
	}
	return (encoded_int_t){ m, n };
}

/* check if a declaration is an array,
 * and if it is, return dimensions */
int check_array(exp_tree_t *decl)
{
	int array = 0;
	int i;
	for (i = 0; i < decl->child_count; ++i)
		if (decl->child[i]->head_type == ARRAY_DIM)
			++array;
	return array;
}

/* 
 * Get a register address to be used temporarily.
 * for example, to evaluate the expression
 *	x + y * z
 * two temporary registers are needed.
 */
int get_temp_storage() {
	if (temp_register > 255) {
		fail("expression stack overflow");
	}
	return temp_register++;
}

/*
 * Reset the temporary registers stack.
 * This is done essentially for any new
 * statement.
 */
void new_temp_storage() {
	temp_register = 255 - EXPR_STACK_SIZE;
}

/*
 * Output the final codegen'd BPF assembly,
 * fit to be sent to the assembler program.
 * An array is used to allow backpatching
 * during code generation.
 */
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

/*
 * Push a compiled line of assembly
 * to the final code array.
 */
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

/*
 * Push a single compiled token to the
 * final code array. This routine is used
 * to push dummy tokens that are later
 * backpatched. It returns a pointer to
 * the address of the token in the 
 * final code array.
 */
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
	if (strcmp(tok, "\n"))
		++byte_count;
	return code_text[code_toks - 1];
}

/* Extract the raw string from a token */
char* get_tok_str(token_t t)
{
	static char buf[1024];
	strncpy(buf, t.start, t.len);
	buf[t.len] = 0;
	return buf;
}

/* Check if a symbol is already defined */
int sym_check(token_t* tok)
{
	int i;
	char *s = get_tok_str(*tok);
	char buf[128];

	for (i = 0; i < 256; i++)
		if (!strcmp(symtab[i], s)) {
			sprintf(buf, "symbol `%s' defined twice", s);
			compiler_fail(buf, tok, 0, 0);
		}
	return 0;
}

/*
 * Allocate a permanent storage register without
 * giving it a symbol name. Useful for special 
 * registers created by the compiler. 
 */
int nameless_perm_storage()
{
	if (syms >= 255 - EXPR_STACK_SIZE)
		fail("out of permanent registers");
	return syms++;
}

/*
 * Create a new symbol, obtaining its permanent
 * storage address.
 */ 
int sym_add(token_t *tok)
{
	char *s = get_tok_str(*tok);
	strcpy(symtab[syms], s);
	if (syms >= 255 - EXPR_STACK_SIZE)
		fail("out of permanent registers");
	return syms++; 
}

/* Lookup storage address of a symbol */
int sym_lookup(token_t* tok)
{
	char buf[1024];
	char *s = get_tok_str(*tok);
	int i = 0;

	for (i = 0; i < syms; i++)
		if (!strcmp(symtab[i], s))
			return i;
	sprintf(buf, "unknown symbol `%s'", s);
	compiler_fail(buf, tok, 0, 0);
}

/* Tree type -> arithmetic operation code */
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

/* Starting point of the codegen */
void run_codegen(exp_tree_t *tree)
{
	int sto;
	char buf[1024];
	extern void count_labels(exp_tree_t tree);
	int i;

	program_ptr = sto = nameless_perm_storage();
	sprintf(buf, "PtrTo %d\n", sto);
	/*
	 * [This makes an assumption about the
	 * VM's implementation of PtrTo about
	 * which the VM spec is ambiguous
	 * (see documentation.txt)]
	 */
	byte_count -= 2;
	push_line(buf);
	count_labels(*tree);

	/* provide easy access to
	 * the special keyboard register */
	strcpy(symtab[0], "_kbreg");

	for (i = 0; i < 32; i++)
		if (!(instr_arg[i] = malloc(32)))
			fail("malloc instr arg string");

	codegen(tree);
}

/* 
 * Look for labels and setup their symbols and
 * initial code. The initial code is a bunch
 * of assignments at the beggining of the
 * compiled program that store the program
 * offsets of each label to their symbol-
 * addresses. These offsets get backpatched
 * in when the main pass of the codegen
 * encounters the labels again after having
 * had generated N bytes of code coming
 * before them.
 */
void count_labels(exp_tree_t tree)
{
	int i, sym;
	char buf[1024];

	if (tree.head_type == LABEL) {
		++label_count;
		if (!sym_check(tree.tok))
			sym = sym_add(tree.tok);
		sprintf(buf, "Do %d 20 1 ", sym);
		push_line(buf);
		label_bp[sym] = push_compiled_token("_");
		push_compiled_token("\n");
		sprintf(buf, "Do %d 40 1 255\n", sym);
		push_line(buf);
		sprintf(buf, "Do %d 20 1 ", sym);
		push_line(buf);
		label_bp_2[sym] = push_compiled_token("_");
		push_compiled_token("\n");
		sprintf(buf, "Do %d 20 2 %d\n", sym, program_ptr);
		push_line(buf);
	}
	for (i = 0; i < tree.child_count; ++i)
		if (tree.child[i]->head_type == BLOCK
		||	tree.child[i]->head_type == LABEL
		||	tree.child[i]->head_type == IF
		||	tree.child[i]->head_type == WHILE)
			count_labels(*(tree.child[i]));
}

/*
 * The core codegen routine. It returns
 * a codegen_t structure which contains
 *	- adr   : the temporary register
 *	 	  at which the runtime evaluation
 *		  value of the tree is stored
 *		  (if the tree is an expression
 *		  with a value);
 *	- bytes : the number of bytes of
 *		  generated code the tree
 *		  has been responsible for.
 *		  For example, the code for an
 *		  else-less "if" skips forward by
 *		  the number of bytes in its
 *		  block subtree if its conditional
 *		  is false. 
 */
codegen_t codegen(exp_tree_t* tree)
{
	int sto;
	int i, j, k;
	int sym;
	int oper;
	int arith;
	int t1, t2, t3, t4, t5;
	char *name;
	int bytesize = 0;
	codegen_t cod, cod2;
	char buf[1024];
	char *bp1, *bp1b;
	char *bp2, *bp2b;
	int while_start;
	int read;
	token_t one = { TOK_INTEGER, "1", 1, 0, 0 };
	exp_tree_t one_tree = new_exp_tree(NUMBER, &one);
	exp_tree_t *new, *new2;
	exp_tree_t *temp, *dc;
	exp_tree_t fake_tree;
	exp_tree_t fake_tree_2;
	extern int adr_microcode(int sto, int read, int set);
	codegen_t seq;

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
		|| tree->head_type == BPF_INSTR) {
		/* clear expression stack */
		new_temp_storage();
	}

	/* comma-separated sequence (e.g. a = 1, b = 2, c = 3) */
	if (tree->head_type == SEQ) {
		for (i = 0; i < tree->child_count; ++i) {
			seq = codegen(tree->child[i]);
			bytesize += seq.bytes;
			/* comma returns the value of the last element */
			sym = seq.adr;
		}
		return (codegen_t){ sym, bytesize };
	}
	
	/* block */
	if (tree->head_type == BLOCK) {
		/* codegen expressions in block */
		for (i = 0; i < tree->child_count; i++)
			bytesize += codegen(tree->child[i]).bytes;
		return (codegen_t){ 0, bytesize };
	}

	/* variable and array declarations */
	if (tree->head_type == INT_DECL) {
		for (i = 0; i < tree->child_count; ++i) {
			dc = tree->child[i];
			/* variable, with optional assignment */
			if (check_array(dc) == 0) {
				/* create the storage and symbol */
				if (!sym_check(dc->child[0]->tok))
					sym = sym_add(dc->child[0]->tok);
				/* write the code for the assignment, if any */
				if (dc->child_count == 2) {
					if (dc->child[1]->head_type == NUMBER) {
						sprintf(buf, "Do %d 10 1 %s\n", sym,
							get_tok_str(*(dc->child[1]->tok)));
						push_line(buf);
						bytesize += 5;
					} else if (dc->child[1]->head_type == VARIABLE) {
						sto = sym_lookup(dc->child[1]->tok);
						sprintf(buf, "Do %d 10 2 %d\n", sym, sto);
						push_line(buf);
						bytesize += 5;
					} else {
						cod = codegen(dc->child[1]);
						sprintf(buf, "Do %d 10 2 %d\n", sym, cod.adr);
						push_line(buf);
						bytesize += cod.bytes + 5;
					}
				}
			/* array declaration */
			} else if (check_array(dc) == 1) {
				/* make a symbol named after the array
				 * at its index 0 */
				if (!sym_check(dc->child[0]->tok))
					(void)sym_add(dc->child[0]->tok);
				/* make nameless storage for the subsequent
				 * indices -- when we deal with an expression
				 * such as array[index] we only need to know
				 * the starting point of the array and the
				 * value "index" evaluates to */
				sto = atoi(get_tok_str(*(dc->child[1]->tok)));
				for (j = 0; j < sto - 1; j++)
					(void)nameless_perm_storage();
			}
		}
		return (codegen_t){ 0, bytesize };
	}

	/* direct instruction */
	if (tree->head_type == BPF_INSTR) {
		name = get_tok_str(*(tree->tok));
		for (i = 0; i < INSTR_COUNT; i++) {
			if (!strcmp(name, instr_info[i].name)) {
				bytesize = 1 + instr_info[i].arg_count;
				for (j = 0; j < instr_info[i].arg_count; j++) {
					if(tree->child[j]->head_type == VARIABLE
						&& instr_info[i].arg_type[j] > 0) {
						/* variable address expected, either to
						 * read from (1) or to assign to (2) */
						sym = sym_lookup(tree->child[j]->tok);
						sprintf(buf, "%d", sym);
						strcpy(instr_arg[j], buf);
					} else if(instr_info[i].arg_type[j] == 0) {
						if (tree->child[j]->head_type == NUMBER)
							strcpy(instr_arg[j], get_tok_str
								(*(tree->child[j]->tok)));
						else
							compiler_fail("number literal expected",
								tree->child[j]->tok, 0, 0);					
					} else if( instr_info[i].arg_type[j] == 2
						&& tree->child[j]->head_type == ARRAY) {
						/* operate on memory location... tricky,
						 * since not directly provided by VM.
						 * could implemented using the synthetic
						 * indirection hack one day 
						 */
						compiler_fail("That's too tough for me ATM",
							tree->child[j]->tok, 0, 0);
					} else {
						cod = codegen(tree->child[j]);
						sprintf(buf, "%d", cod.adr);
						strcpy(instr_arg[j], buf);
						bytesize += cod.bytes;
					}
				}
				push_compiled_token(instr_info[i].instr);
				for (j = 0; j < instr_info[i].arg_count; j++)
					push_compiled_token(instr_arg[j]);
				push_compiled_token("\n");
				return (codegen_t) {0, bytesize};
			}
		}
	}

	/* pre-increment, pre-decrement of variable lvalue */
	if ((tree->head_type == INC
		|| tree->head_type == DEC)
		&& tree->child[0]->head_type == VARIABLE) {
		sym = sym_lookup(tree->child[0]->tok);
		sprintf(buf, "Do %d %d 1 1\n", sym,
			tree->head_type == INC ? 20 : 30);
		push_line(buf);
		return (codegen_t) { sym, 5 };
	}

	/* pre-increment, pre-decrement of array lvalue */
	if ((tree->head_type == INC
		|| tree->head_type == DEC)
		&& tree->child[0]->head_type == ARRAY) {
		/* rewrite ++bob[haha] as (bob[haha] += 1); 
		 * and go codegen that */
		fake_tree_2 = new_exp_tree(tree->head_type == 
			INC ? ADD : SUB, NULL);
		add_child(&fake_tree_2, tree->child[0]);
		add_child(&fake_tree_2, alloc_exptree(one_tree));
		fake_tree = new_exp_tree(ASGN, NULL);
		add_child(&fake_tree, tree->child[0]);
		add_child(&fake_tree, alloc_exptree(fake_tree_2));
		return codegen(&fake_tree);
	}

	/* post-increment, post-decrement of variable lvalue */
	if ((tree->head_type == POST_INC
		|| tree->head_type == POST_DEC)
		&& tree->child[0]->head_type == VARIABLE) {
		sym = sym_lookup(tree->child[0]->tok);
		sto = get_temp_storage();
		/* store the variable's value to temp
		 * storage then bump it and return
		 * the temp storage */
		sprintf(buf, "Do %d 10 2 %d\n", sto, sym);
		push_line(buf);
		sprintf(buf, "Do %d %d 1 1\n", sym,
			tree->head_type == POST_INC ? 20 : 30);
		push_line(buf);
		return (codegen_t) { sto, 10 };
	}

	/* post-decrement, post-decrement of array lvalue */
	if ((tree->head_type == POST_INC
		|| tree->head_type == POST_DEC)
		&& tree->child[0]->head_type == ARRAY) {
		/* given bob[haha]++,
		 * codegen bob[haha], keep its value,
		 * codegen ++bob[haha]; 
		 * and give back the kept value */
		cod = codegen(tree->child[0]);
		bytesize += cod.bytes;
		fake_tree = new_exp_tree(tree->head_type == 
			POST_INC ? INC : DEC, NULL);
		add_child(&fake_tree, tree->child[0]);
		bytesize += codegen(&fake_tree).bytes;
		return (codegen_t) { cod.adr, bytesize};
	}

	/* simple variable assignment */
	if (tree->head_type == ASGN && tree->child_count == 2
		&& tree->child[0]->head_type == VARIABLE) {
		sym = sym_lookup(tree->child[0]->tok);
		if (tree->child[1]->head_type == NUMBER) {
			/* optimized code for number operand */
			sprintf(buf, "Do %d 10 1 %s\n", sym,
				get_tok_str(*(tree->child[1]->tok)));
			push_line(buf);
			return (codegen_t){ sym, 5 };
		} else if (tree->child[1]->head_type == VARIABLE) {
			/* optimized code for variable operand */
			sto = sym_lookup(tree->child[1]->tok);
			sprintf(buf, "Do %d 10 2 %d\n", sym, sto);
			push_line(buf);
			return (codegen_t){ sym, 5 };
		} else {
			/* general case */
			cod = codegen(tree->child[1]);
			sprintf(buf, "Do %d 10 2 %d\n", sym, cod.adr);
			push_line(buf);
			return (codegen_t){ sym, cod.bytes + 5 };
		}
	}

	/* array assignment */
	if (tree->head_type == ASGN && tree->child_count == 2
		&& tree->child[0]->head_type == ARRAY) {
		sto = get_temp_storage();	/* microcode store register */
		sym = sym_lookup(tree->child[0]->child[0]->tok);
		/* get index expression */
		cod = codegen(tree->child[0]->child[1]);
		/* compute complete address of array cell */
		sprintf(buf, "Do %d 10 1 %d\n", sto, sym);
		push_line(buf);
		sprintf(buf, "Do %d 20 2 %d\n", sto, cod.adr);
		push_line(buf);
		/* right operand */
		cod2 = codegen(tree->child[1]);
		bytesize += cod.bytes + cod2.bytes + 10;
		bytesize += adr_microcode(sto, cod2.adr, 1);
		return (codegen_t) { cod2.adr, bytesize };
	}

	/* if */
	if (tree->head_type == IF) {
		/* codegen the conditional */
		cod = codegen(tree->child[0]);
		/* now write the code that stores
		 * the jump offset to a temporary
	 	 * register "sto" */
		sto = get_temp_storage();
		sprintf(buf, "Do %d 10 1 ", sto);
		push_line(buf);
		bp1 = push_compiled_token("_");
		push_compiled_token("\n");
		sprintf(buf, "Do %d 40 1 255\n", sto);
		push_line(buf);
		sprintf(buf, "Do %d 20 1 ", sto);
		push_line(buf);
		bp1b = push_compiled_token("_");
		push_compiled_token("\n");
		/* branch if the conditional is false */
		sprintf(buf, "zbPtrTo %d 0 %d\n", cod.adr, sto);
		push_line(buf);
		bytesize += codegen(tree->child[1]).bytes;
		if (tree->child_count == 3) {
			bytesize += 24;
		}
		/* backpatch in the jump offset */
		sprintf(buf, "%d", encode(bytesize).mult);
		strcpy(bp1, buf);
		sprintf(buf, "%d", encode(bytesize).mod);
		strcpy(bp1b, buf);

		bytesize += 19 + cod.bytes;

		/* else ? */
		if (tree->child_count == 3) {
			sto = get_temp_storage();
			sym = get_temp_storage();
			/* in the first block, write
			 * code that jumps over the "else"
			 * block */
			sprintf(buf, "PtrTo %d\n", sto);
			push_line(buf);
			sprintf(buf, "Do %d 10 1 ", sym);
			push_line(buf);
			bp2 = push_compiled_token("_");
			push_compiled_token("\n");
			sprintf(buf, "Do %d 40 1 255\n", sym);
			push_line(buf);
			sprintf(buf, "Do %d 20 1 ", sym);
			push_line(buf);
			bp2b = push_compiled_token("_");
			push_compiled_token("\n");
			sprintf(buf, "Do %d 20 2 %d\n", sto, sym);
			push_line(buf);
			sprintf(buf, "PtrFrom %d\n", sto);
			push_line(buf);
			cod = codegen(tree->child[2]);
			/* backpatch */
			sprintf(buf, "%d", encode(22 + cod.bytes).mult);
			strcpy(bp2, buf);
			sprintf(buf, "%d", encode(22 + cod.bytes).mod);
			strcpy(bp2b, buf);
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
		/* store jump offset */
		sto = get_temp_storage();
		sprintf(buf, "Do %d 10 1 ", sto);
		push_line(buf);
		bp1 = push_compiled_token("_");
		push_compiled_token("\n");
		sprintf(buf, "Do %d 40 1 255\n", sto);
		push_line(buf);
		sprintf(buf, "Do %d 20 1 ", sto);
		push_line(buf);
		bp1b = push_compiled_token("_");
		push_compiled_token("\n");
		/* conditonal branch */
		sprintf(buf, "zbPtrTo %d 0 %d\n", cod.adr, sto);
		push_line(buf);
		bytesize += codegen(tree->child[1]).bytes;
		/* loop back to right before the conditional */
		sprintf(buf, "PtrFrom %d\n", while_start);
		push_line(buf);
		bytesize += 2;
		/* backpatch in the jump offset size */
		sprintf(buf, "%d", encode(bytesize).mult);
		strcpy(bp1, buf);
		sprintf(buf, "%d", encode(bytesize).mod);
		strcpy(bp1b, buf);

		bytesize += 21 + cod.bytes;

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
		sym = sym_lookup(tree->tok);
		sprintf(buf, "Do %d 10 2 %d\n", 
			sto,
			sym);
		push_line(buf);
		return (codegen_t) { sto, 5 };
	}

	/* label */
	if (tree->head_type == LABEL) {
		sym = sym_lookup(tree->tok);
		/* more backpatch magic */
		sprintf(buf, "%d", encode(byte_count).mult);
		strcpy(label_bp[sym], buf);
		sprintf(buf, "%d", encode(byte_count).mod);
		strcpy(label_bp_2[sym], buf);
		return (codegen_t) { 0, 0 };
	}

	/* goto */
	if (tree->head_type == GOTO) {
		sym = sym_lookup(tree->tok);
		sprintf(buf, "PtrFrom %d\n", sym);
		push_line(buf);
		return (codegen_t) { 0, 2 };
	}

	/* array value fetching */
	if (tree->head_type == ARRAY) {
		sto = get_temp_storage();	/* microcode input register */
		read = get_temp_storage();	/* microcode output register */
		sym = sym_lookup(tree->child[0]->tok);
		/* get index expression */
		cod = codegen(tree->child[1]);
		/* compute complete address of array cell */
		sprintf(buf, "Do %d 10 1 %d\n", sto, sym);
		push_line(buf);
		sprintf(buf, "Do %d 20 2 %d\n", sto, cod.adr);
		push_line(buf);
		bytesize += cod.bytes + 10;
		bytesize += adr_microcode(sto, read, 0);
		return (codegen_t) { read, bytesize };
	}

	/* arithmetic */
	if ((arith = arith_op(tree->head_type)) && tree->child_count) {
		/* (with optimized code for number and variable operands
		 * that avoids wasting temporary registes) */
		sto = get_temp_storage();
		for (i = 0; i < tree->child_count; i++) {
			oper = i ? arith : 10;
			if (tree->child[i]->head_type == NUMBER) {
				sprintf(buf, "Do %d %d 1 %s\n", sto, oper,
					get_tok_str(*(tree->child[i]->tok)));
				push_line(buf);
				bytesize += 5;
			} else if(tree->child[i]->head_type == VARIABLE) {
				sym = sym_lookup(tree->child[i]->tok);
				sprintf(buf, "Do %d %d 2 %d\n", sto, oper, sym);
				push_line(buf);
				bytesize += 5;
			} else {
				cod = codegen(tree->child[i]);
				sprintf(buf, "Do %d %d 2 %d\n", sto, oper,
					cod.adr);
				push_line(buf);
				bytesize += 5 + cod.bytes;
			}
		}
		return (codegen_t){ sto, bytesize };
	}

	/* == and != */
	if (tree->head_type == EQL || tree->head_type == NEQL) {
		t1 = get_temp_storage();
		t2 = get_temp_storage();
		t3 = get_temp_storage();
		t4 = get_temp_storage();
		t5 = get_temp_storage();
		
		/* t3 = a - b */
		/* t4 = b - a */
		for (i = 0; i < 2; i++) {
			for (j = 0; j < tree->child_count; j++) {
				oper = j ? 30 : 10;
				sto = i ? t4 : t3;
				k = i ? 1 - j : j;
				if (tree->child[k]->head_type == NUMBER) {
					sprintf(buf, "Do %d %d 1 %s\n",
					  sto, oper,
					  get_tok_str(*(tree->child[k]->tok)));
					push_line(buf);
					bytesize += 5;
				} else if(tree->child[k]->head_type 
					== VARIABLE) {
					sym = sym_lookup(tree->child[k]->tok);
					sprintf(buf, "Do %d %d 2 %d\n", 
						sto, oper, sym);
					push_line(buf);
					bytesize += 5;
				} else {
					cod = codegen(tree->child[k]);
					sprintf(buf, "Do %d %d 2 %d\n", 
						sto, oper, 
						cod.adr);
					push_line(buf);
					bytesize += 5 + cod.bytes;
				}
			}
		}

		sprintf(buf, "Do %d 10 1 9\n", t1);
		push_line(buf);
		sprintf(buf, "Do %d 10 1 5\n", t2);
		push_line(buf);
		sprintf(buf, "Do %d 10 1 %d\n", t5, 
			tree->head_type == EQL);
		push_line(buf);
		sprintf(buf, "zbPtrTo %d 0 %d\n", t3, t1);
		push_line(buf);
		sprintf(buf, "zbPtrTo %d 0 %d\n", t4, t2);
		push_line(buf);
		sprintf(buf, "Do %d 10 1 %d\n",
			t5, tree->head_type != EQL);
		push_line(buf);
		bytesize += 28;

		return (codegen_t){ t5, bytesize };
	}

	if (tree->head_type == PROC)
		compiler_fail("BPF codegen does not support procedures",
		tree->tok, 0, 0);
		

	fprintf(stderr, "Sorry, I can't yet codegen this tree: \n");
	fflush(stderr);
	printout_tree(*tree);
	fputc('\n', stderr);
	exit(1);
}

/* Synthetic indirection */
int adr_microcode(int sto, int read, int set)
{
	int i;
	char buf[128];
	int temp1, temp2, temp3;
	temp1 = get_temp_storage();	/* 232 */
	temp2 = get_temp_storage();	/* 231 */
	temp3 = get_temp_storage();	/* 230 */

	/* AdrSet / AdrGet hack for BPF 1.0
	 * originally coded July 15-16, 2008 by blockeduser
	 */

	sprintf(buf, "Do %d 10 1 255\n", temp1);
	push_line(buf);
	sprintf(buf, "Do %d 40 1 7\n", temp1);
	push_line(buf);
	sprintf(buf, "Do %d 20 1 12\n", temp1);
	push_line(buf);

	sprintf(buf, "Do %d 10 2 %d\n", temp2, sto);
	push_line(buf);
	sprintf(buf, "Do %d 30 1 1\n", temp2);
	push_line(buf);
	sprintf(buf, "Do %d 40 1 7\n", temp2);
	push_line(buf);
	sprintf(buf, "Do %d 20 1 12\n", temp2);
	push_line(buf);
	sprintf(buf, "PtrTo %d\n", temp3);
	push_line(buf);
	sprintf(buf, "Do %d 20 2 %d\n", temp2, temp3);
	push_line(buf);
	sprintf(buf, "Do %d 20 2 %d\n", temp1, temp3);
	push_line(buf);
	sprintf(buf, "PtrFrom %d\n", temp2);
	push_line(buf);

	i = 0;
	while(i++ < 255)
	{
		if (set)
			sprintf(buf, "Do %d 10 2 %d\n", i, read);
		else
			sprintf(buf, "Do %d 10 2 %d\n", read, i);
		push_line(buf);
		sprintf(buf, "PtrFrom %d\n", temp1);
		push_line(buf);
	}

	return 1834;	/* 1834 bytes of compiled code */
}


