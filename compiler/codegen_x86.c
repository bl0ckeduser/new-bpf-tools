/* Syntax tree -> 32-bit x86 assembly (GAS syntax) generator 
 * (it's my first time [quickly] doing x86 so it's probably far
 * from optimal). The essence of this module is juggling with
 * registers and coping with permitted parameter-types. 
 *		=>	Many improvements possible.	<=
 */

/* NOTE: this code generator uses the standard C boolean
 * idiom of 0 = false, nonzero = true, unlike the BPFVM one.
 */

#include "tree.h"
#include "tokens.h"
#include "codegen_x86.h"
#include <string.h>
#include <stdio.h>

char current_proc[64];
char symtab[256][32] = {""};
char arg_symtab[256][32] = {""};
char str_const_tab[256][32] = {""};
char ts_used[TEMP_REGISTERS];
char tm_used[TEMP_MEM];
int syms = 0;
int arg_syms = 0;
int str_consts = 0;
int temp_register = 0;
int swap = 0;
int proc_ok = 1;
int else_ret;
int main_defined = 0;
char entry_buf[1024], buf[1024];
int ccid = 0;

int stack_size;
int intl_label = 0; /* internal label numbering */

extern void fail(char*);
extern void compiler_fail(char *message, token_t *token,
	int in_line, int in_chr);

void print_code() {
	;
}

/* make all the general-purpose registers
 * available for temporary use */
void new_temp_reg() {
	int i;
	for (i = 0; i < TEMP_REGISTERS; ++i)
		ts_used[i] = 0;
}

/* get a general-purpose register for
 * temporary use */
char* get_temp_reg() {
	int i;
	for (i = 0; i < TEMP_REGISTERS; ++i)
		if (!ts_used[i]) {
			ts_used[i] = 1;
			return temp_reg[i];
		}
	fail("out of registers");
}

/* let go of a temporary register */
void free_temp_reg(char *reg) {
	int i;

	for (i = 0; i < TEMP_REGISTERS; ++i)
		if (!strcmp(reg, temp_reg[i]))
			ts_used[i] = 0;
}

/* make all the stack-based temporary
 * storage available for use */
void new_temp_mem() {
	int i;
	for (i = 0; i < TEMP_MEM; ++i)
		tm_used[i] = 0;
}

/* use some stack-based temporary
 * storage */
char* get_temp_mem() {
	int i;
	for (i = 0; i < TEMP_MEM; ++i)
		if (!tm_used[i]) {
			tm_used[i] = 1;
			return temp_mem[i];
		}
	fail("out of temporary memory");
}

/* let go of temporary stack memory */
void free_temp_mem(char *reg) {
	int i;

	for (i = 0; i < TEMP_MEM; ++i)
		if (!strcmp(reg, temp_mem[i]))
			tm_used[i] = 0;
}

/* get the GAS syntax for a symbol
 * stored in the stack */
char *symstack(int id) {
	static char buf[128];
	if (!id)
		sprintf(buf, "(%%ebp)");
	else if (id < 0)
		sprintf(buf, "%d(%%ebp)", -id * 4);
	else
		sprintf(buf, "-%d(%%ebp)", id * 4);
	return buf;
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

	for (i = 0; i < syms; i++)
		if (!strcmp(symtab[i], s)) {
			sprintf(buf, "symbol `%s' defined twice", s);
			compiler_fail(buf, tok, 0, 0);
		}

	for (i = 0; i < arg_syms; i++)
		if (!strcmp(arg_symtab[i], s)) {
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

	for (i = 0; i < arg_syms; i++)
		if (!strcmp(arg_symtab[i], s))
			return -2 - i;

	sprintf(buf, "unknown symbol `%s'", s);
	compiler_fail(buf, tok, 0, 0);
}

/* Lookup storage number of a string constant */
int str_const_lookup(token_t* tok)
{
	char buf[1024];
	char *s = get_tok_str(*tok);
	int i = 0;

	for (i = 0; i < str_consts; i++)
		if (!strcmp(str_const_tab[i], s))
			return i;

	sprintf(buf, "unregistered string constant `%s'", s);
	compiler_fail(buf, tok, 0, 0);
}

/* Add a string constant to the string constant table */
int str_const_add(token_t *tok)
{
	char *s = get_tok_str(*tok);
	strcpy(str_const_tab[str_consts], s);
	return str_consts++; 
}

/* Tree type -> arithmetic routine */
char* arith_op(int ty)
{
	switch (ty) {
		case ADD:
			return "addl";
		case SUB:
			return "subl";
		case MULT:
			return "imull";
		default:
			return 0;
	}
}

/*
 * Given its name, the names of its arguments,
 * and the tree containing its body block,
 * write the code for a procedure, and set up
 * the memory, stack, symbols, etc. needed.
 */
void codegen_proc(char *name, exp_tree_t *tree, char **args)
{
	extern void setup_symbols(exp_tree_t* tree);
	char buf[1024];
	char *buf2;
	int i;

	syms = 1;
	arg_syms = 0;

	strcpy(current_proc, name);

	/* argument symbols */
	for (i = 0; args[i]; ++i)
		strcpy(arg_symtab[arg_syms++], args[i]);

	/* make the symbol and label for the procedure */
#ifndef MINGW_BUILD
	printf(".type %s, @function\n", name);
#endif
	printf("%s:\n", name);

	if (!strcmp(name, "main"))
		main_defined = 1;

	/* setup the identifier symbol table and make stack
	 * space for all the variables in the program */
	*entry_buf = 0;
	setup_symbols(tree);

	for (i = 0; i < TEMP_MEM; ++i) {
		buf2 = malloc(64);
		strcpy(buf2, symstack(nameless_perm_storage()));
		temp_mem[i] = buf2;
	}

	/* do the usual x86 function entry process,
	 * and set aside stack memory for local
	 * variables */
	printf("# set up stack space\n");
	printf("pushl %%ebp\n");
	printf("movl %%esp, %%ebp\n");
	printf("subl $%d, %%esp\n", syms * 4);
	printf("\n# >>> compiled code for '%s'\n", name);

	printf("_tco_%s:\n", name);	/* hook for TCO */
	puts(entry_buf);

	/* code the body of the procedure */
	codegen(tree);

	/* typical x86 function return */
	printf("\n# clean up stack and return\n");
	printf("_ret_%s:\n", name);	/* hook for value returns */
	printf("addl $%d, %%esp\n", syms * 4);
	printf("movl %%ebp, %%esp\n");
	printf("popl %%ebp\n");
	printf("ret\n\n");
}

/* Starting point of the codegen */
void run_codegen(exp_tree_t *tree)
{
	char *main_args[] = { NULL };
	extern void deal_with_procs(exp_tree_t *tree);
	extern void deal_with_str_consts(exp_tree_t *tree);

#ifdef MINGW_BUILD
	char main_name[] = "_main";
#else
	char main_name[] = "main";
#endif

	printf(".section .rodata\n");
	printf("_echo_format: .string \"%%d\\n\"\n");

	deal_with_str_consts(tree);

	printf(".section .text\n");
	printf(".globl %s\n\n", main_name);

	/* 
	 * We deal with the procedures separately
	 * before doing any further work to prevent
	 * their code from being output within
	 * the code for "main"
	 */
	proc_ok = 1;
	deal_with_procs(tree);
	proc_ok = 0;	/* no further procedures shall
			 * be allowed */

	/*
	 * If the user code contains no main(),
	 * compile the main lexical body as the
	 * the main-code.
	 */
	if (!main_defined)
		codegen_proc(main_name, tree, main_args);

	/* echo utility routine */
#ifndef MINGW_BUILD
	printf(".type echo, @function\n");
#endif
	printf("echo:\n");
    printf("pushl $0\n");
    printf("pushl 8(%%esp)\n");
    printf("pushl $_echo_format\n");
#ifdef MINGW_BUILD
    printf("call _printf\n");
#else
    printf("call printf\n");
#endif
    printf("addl $12, %%esp  # get rid of the printf args\n");
    printf("ret\n");
}

void deal_with_str_consts(exp_tree_t *tree)
{
	int i;
	int id;

	if (tree->head_type == STR_CONST) {
		id = str_const_add(tree->tok);
		/* the quotes are part of the token string */
		printf("_str_const_%d: .string %s\n", 
			id, get_tok_str(*(tree->tok)));

		/* TODO: escape sequences like \n ... */
	}

	for (i = 0; i < tree->child_count; ++i)
		deal_with_str_consts(tree->child[i]);
}


/* See run_codegen above */
void deal_with_procs(exp_tree_t *tree)
{
	int i;

	if (tree->head_type == PROC) {
		codegen(tree);
		(*tree).head_type = NULL_TREE;
	}

	for (i = 0; i < tree->child_count; ++i)
		if (tree->child[i]->head_type == BLOCK
		|| tree->child[i]->head_type == PROC)
			deal_with_procs(tree->child[i]);
}

/* See codegen_proc above */
void setup_symbols(exp_tree_t *tree)
{
	int i, sto;
	char *str;

	/* variable declaration, with optional assignment */
	if (tree->head_type == INT_DECL) {
		/* create the storage and symbol */
		if (!sym_check(tree->child[0]->tok))
			(void)sym_add(tree->child[0]->tok);
		/* preserve assignment, else discard tree */
		if (tree->child_count == 2)
			(*tree).head_type = ASGN;
		else
			(*tree).head_type = NULL_TREE;
	}

	/* array declaration */
	if (tree->head_type == ARRAY_DECL) {
		/* number of elements (integer) */
		sto = atoi(get_tok_str(*(tree->child[1]->tok)));
		/* make a symbol named after the array
		 * and store the *ADDRESS* of its first element there */
		if (!sym_check(tree->child[0]->tok))
			(void)sym_add(tree->child[0]->tok);
		str = get_temp_reg();
		sprintf(buf, "leal %s, %s\n",
			symstack(sym_lookup(tree->child[0]->tok) + sto),
			str);
		strcat(entry_buf, buf);
		sprintf(buf, "movl %s, %s\n",
			str,
			symstack(sym_lookup(tree->child[0]->tok)));
		strcat(entry_buf, buf);
		/* make nameless storage for the subsequent
		 * indices -- when we deal with an expression
		 * such as array[index] we only need to know
		 * the starting point of the array and the
		 * value "index" evaluates to */
		for (i = 0; i < sto; i++)
			(void)nameless_perm_storage();
		/* discard tree */
		*tree = null_tree;
	}

	for (i = 0; i < tree->child_count; ++i)
		if (tree->child[i]->head_type == BLOCK
		||	tree->child[i]->head_type == IF
		||	tree->child[i]->head_type == WHILE
		||	tree->child[i]->head_type == INT_DECL
		||	tree->child[i]->head_type == ARRAY_DECL)
			setup_symbols(tree->child[i]);
}

/*
 * If a temporary storage location is already
 * a register, give it back as is. Otherwise,
 * copy it to a new temporary register and give
 * this new register.
 */
char* registerize(char *stor)
{
	int is_reg = 0;
	int i;
	char* str;

	for (i = 0; i < TEMP_REGISTERS; ++i)
		if (!strcmp(stor, temp_reg[i]))
			return stor;
	
	str = get_temp_reg();
	printf("movl %s, %s\n", stor, str);
	return str;
}

/*
 * The core codegen routine. It returns
 * the temporary register at which the runtime
 * evaluation value of the tree is stored
 * (if the tree is an expression
 * with a value)
 */
char* codegen(exp_tree_t* tree)
{
	char *sto, *sto2, *sto3, *sto4;
	char *str, *str2;
	char *name;
	char *proc_args[32];
	char my_ts_used[TEMP_REGISTERS];
	char *buf;
	int i;
	int sym;
	char *oper;
	char *arith;
	token_t one = { TOK_INTEGER, "1", 1, 0, 0 };
	exp_tree_t one_tree = new_exp_tree(NUMBER, &one);
	exp_tree_t fake_tree;
	exp_tree_t fake_tree_2;
	extern char* cheap_relational(exp_tree_t* tree, char *oppcheck);
	extern char* optimized_if(exp_tree_t* tree, char *oppcheck);
	extern char* optimized_while(exp_tree_t* tree, char *oppcheck);
	int lab1, lab2;
	char *sav1, *sav2;
	int my_ccid;

	if (tree->head_type == BLOCK
		|| tree->head_type == IF
		|| tree->head_type == WHILE
		|| tree->head_type == BPF_INSTR) {
		/* clear temporary memory and registers */
		new_temp_mem();
		new_temp_reg();
	}

	/* "bob123" */
	if (tree->head_type == STR_CONST) {
		sto = get_temp_reg();
		printf("movl $_str_const_%d, %s\n",
			str_const_lookup(tree->tok),
			sto);
		return sto;
	}

	/* &x */
	if (tree->head_type == ADDR
		&& tree->child_count == 1
		&& tree->child[0]->head_type == VARIABLE) {
		
		sto = get_temp_reg();

		/* LEA: load effective address */
		printf("leal %s, %s\n",
				symstack(sym_lookup(tree->child[0]->tok)),
				sto);

		return sto;
	}

	/* &(a[v]) = a + 4 * v */
	if (tree->head_type == ADDR
		&& tree->child_count == 1
		&& tree->child[0]->head_type == ARRAY) {
		
		sto = get_temp_reg();

		printf("movl %s, %s\n",
				symstack(sym_lookup(tree->child[0]->child[0]->tok)),
				sto);

		sto2 = registerize(codegen(tree->child[0]->child[1]));
		printf("imull $4, %s\n", sto2);
		printf("addl %s, %s\n", sto2, sto);

		return sto;
	}


	/* *(exp) */
	if (tree->head_type == DEREF
		&& tree->child_count == 1) {
		
		sto = registerize(codegen(tree->child[0]));
		sto2 = get_temp_reg();
		printf("movl (%s), %s\n", sto, sto2);
		
		return sto2;
	}

	/* procedure call */
	if (tree->head_type == PROC_CALL) {
		/* push all the registers being used */
		for (i = 0; i < TEMP_REGISTERS; ++i)
			if (ts_used[i])
				printf("pushl %s\t# save temp register\n",
					temp_reg[i]);
		memcpy(my_ts_used, ts_used, TEMP_REGISTERS);

		/* push the arguments in reverse order */
		for (i = tree->child_count - 1; i >= 0; --i) {
			sto = codegen(tree->child[i]);
			printf("pushl %s\t# argument %d to %s\n", sto, i,
				 get_tok_str(*(tree->tok)));
			free_temp_reg(sto);
		}

		/* call that routine */
		printf("call %s\n", get_tok_str(*(tree->tok)));

		/* throw off the arguments from the stack */
		printf("addl $%d, %%esp\t# throw off %d arg%s\n",
			4 * tree->child_count, tree->child_count,
			tree->child_count > 1 ? "s" : "");

		/* move the return-value register (EAX)
		 * to temporary stack-based storage */
		sto = get_temp_mem();
		printf("movl %%eax, %s\n", sto);

		/* restore the registers as they were
		 * prior to the call -- pop them in 
		 * reverse order they were pushed */
		for (i = 0; i < TEMP_REGISTERS; ++i)
			if (my_ts_used[TEMP_REGISTERS - (i + 1)])
				printf("popl %s\t# restore temp register\n",
					temp_reg[TEMP_REGISTERS - (i + 1)]);

		/* give back the temporary stack storage
		 * with the return value in it */
		return sto;
	}

	/* procedure */
	if (tree->head_type == PROC) {
		if (!proc_ok)
			compiler_fail("proc not allowed here", tree->tok, 0, 0);

		/* put the list of arguments in char *proc_args */
		if (tree->child[0]->child_count) {
			for (i = 0; tree->child[0]->child[i]; ++i) {
				buf = malloc(64);
				strcpy(buf, get_tok_str
					(*(tree->child[0]->child[i]->tok)));
				proc_args[i] = buf;
			}
			proc_args[i] = NULL;
		} else
			*proc_args = NULL;

		/* can't nest procedure definitions */
		proc_ok = 0;

		/* do the boilerplate routine stuff and code
		 * its body */
		buf = malloc(64);
		strcpy(buf, get_tok_str(*(tree->tok)));
		codegen_proc(buf,
			tree->child[1],
			proc_args);
		free(buf);

		proc_ok = 1;

		return NULL;
	}

	/* TCO return ? */
	if (tree->head_type == RET
	&& tree->child[0]->head_type == PROC_CALL
	&& !strcmp(get_tok_str(*(tree->child[0]->tok)),
		current_proc)) {
		/* aha ! I can TCO this */
		printf("# TCO'd self-call\n");

		new_temp_reg();
		new_temp_mem();

		/* move the arguments directly to the
		 * callee stack offsets */
		for (i = tree->child[0]->child_count - 1; i >= 0; --i) {
			sto = codegen(tree->child[0]->child[i]);
			str = registerize(sto);
			printf("movl %s, %s\n", str, symstack(-2 - i));
			free_temp_reg(str);
		}

		printf("jmp _tco_%s\n", current_proc);
		return NULL;
	}

	/* return */
	if (tree->head_type == RET) {
		new_temp_reg();
		new_temp_mem();
		/* code the return expression */
		sto = codegen(tree->child[0]);
		/* put the return expression's value
		 * in EAX and jump to the end of the
		 * routine */
		printf("# return value\n");
		if (strcmp(sto, "%eax"))
			printf("movl %s, %%eax\n", sto, str);
		printf("jmp _ret_%s\n", current_proc);
		return NULL;
	}

	/* relational operators */
	if (tree->head_type == LT)
		return cheap_relational(tree, "jge");
	if (tree->head_type == LTE)
		return cheap_relational(tree, "jg");
	if (tree->head_type == GT)
		return cheap_relational(tree, "jle");
	if (tree->head_type == GTE)
		return cheap_relational(tree, "jl");
	if (tree->head_type == EQL)
		return cheap_relational(tree, "jne");
	if (tree->head_type == NEQL)
		return cheap_relational(tree, "je");

	/* special routines */
	if (tree->head_type == BPF_INSTR) {
		name = get_tok_str(*(tree->tok));
		if (!strcmp(name, "echo")) {
			if (tree->child[0]->head_type == NUMBER) {
				/* optimized code for number operand */
				printf("pushl $%s\n",
					get_tok_str(*(tree->child[0]->tok)));
			} else if (tree->child[0]->head_type == VARIABLE) {
				/* optimized code for variable operand */
				sto = symstack(sym_lookup(tree->child[0]->tok));
				printf("pushl %s\n", sto);
			} else {
				/* general case */
				sto = codegen(tree->child[0]);
				printf("pushl %s\n", sto);
				free_temp_reg(sto);
			}
			printf("call echo\n");
			return NULL;
		}
	}
	
	/* block */
	if (tree->head_type == BLOCK) {
		/* codegen expressions in block */
		for (i = 0; i < tree->child_count; i++) {
			if (tree->child[i]->head_type == ASGN) {
				/* clear temporary memory and registers */
				new_temp_mem();
				new_temp_reg();
			}
			codegen(tree->child[i]);
		}
		return NULL;
	}

	/* ! -- logical not (0 if X is not 1) */
	if (tree->head_type == CC_NOT) {
		my_ccid = ccid++;
		sto = codegen(tree->child[0]);
		sto2 = get_temp_reg();
		printf("movl $0, %s\n", sto2);
		printf("cmp %s, %s\n", sto, sto2);
		printf("movl $1, %s\n", sto2);
		free_temp_reg(sto);
		printf("je cc%d\n", my_ccid);
		printf("movl $0, %s\n", sto2);
		printf("cc%d:\n", my_ccid);
		return sto2;
	}

	/* && - and with short-circuit */
	if (tree->head_type == CC_AND) {
		my_ccid = ccid++;
		sto2 = get_temp_reg();
		printf("movl $0, %s\n", sto2);
		for (i = 0; i < tree->child_count; ++i) {
			sto = codegen(tree->child[i]);
			printf("cmpl %s, %s\n", sto, sto2);
			printf("je cc%d\n", my_ccid);
			free_temp_reg(sto);
		}
		printf("movl $1, %s\n", sto2);
		printf("cc%d:\n", my_ccid);
		return sto2;
	}

	/* || - or with short-circuit */
	if (tree->head_type == CC_OR) {
		my_ccid = ccid++;
		sto2 = get_temp_reg();
		printf("movl $0, %s\n", sto2);
		for (i = 0; i < tree->child_count; ++i) {
			sto = codegen(tree->child[i]);
			printf("cmpl %s, %s\n", sto, sto2);
			printf("jne cc%d\n", my_ccid);
			free_temp_reg(sto);
		}
		printf("movl $0, %s\n", sto2);
		printf("jmp cc%d_2\n", my_ccid);
		printf("cc%d:\n", my_ccid);
		printf("movl $1, %s\n", sto2);
		printf("cc%d_2:\n", my_ccid);
		return sto2;
	}

	/* pre-increment, pre-decrement of variable lvalue */
	if ((tree->head_type == INC
		|| tree->head_type == DEC)
		&& tree->child[0]->head_type == VARIABLE) {
		sym = sym_lookup(tree->child[0]->tok);
		printf("%s %s\n", tree->head_type == INC ?
			"incl" : "decl", symstack(sym));
		return symstack(sym);
	}

	/* pre-increment, pre-decrement of array lvalue */
	if ((tree->head_type == INC
		|| tree->head_type == DEC)
		&& tree->child[0]->head_type == ARRAY) {

		/* head address */
		sym = sym_lookup(tree->child[0]->child[0]->tok);

		/* index expression */
		str = codegen(tree->child[0]->child[1]);
		sto2 = registerize(str);

		/* build pointer */
		sto = get_temp_reg();
		printf("imull $4, %s\n", sto2);
		printf("addl %s, %s\n", symstack(sym), sto2);
		printf("movl %s, %s\n", sto2, sto);
		free_temp_reg(sto2);

		/* write the final move */
		sto3 = get_temp_reg();
		printf("%s (%s)\n", 
			tree->head_type == DEC ? "decl" : "incl",
			sto);
		printf("movl (%s), %s\n", sto, sto3);
		free_temp_reg(sto);
		return sto3;
	}

	/* post-increment, post-decrement of variable lvalue */
	if ((tree->head_type == POST_INC
		|| tree->head_type == POST_DEC)
		&& tree->child[0]->head_type == VARIABLE) {
		sym = sym_lookup(tree->child[0]->tok);
		sto = get_temp_reg();
		/* store the variable's value to temp
		 * storage then bump it and return
		 * the temp storage */
		printf("movl %s, %s\n", symstack(sym), sto);
		printf("%s %s\n",
			tree->head_type == POST_INC ? "incl" : "decl",
			symstack(sym));
		return sto;
	}

	/* post-decrement, post-decrement of array lvalue */
	if ((tree->head_type == POST_INC
		|| tree->head_type == POST_DEC)
		&& tree->child[0]->head_type == ARRAY) {
		/* given bob[haha]++,
		 * codegen bob[haha], keep its value,
		 * codegen ++bob[haha]; 
		 * and give back the kept value */
		sto = codegen(tree->child[0]);
		fake_tree = new_exp_tree(tree->head_type == 
			POST_INC ? INC : DEC, NULL);
		add_child(&fake_tree, tree->child[0]);
		codegen(&fake_tree);
		return sto;
	}

	/* pointer assignment, e.g. "*ptr = 123" */
	if (tree->head_type == ASGN && tree->child_count == 2
		&& tree->child[0]->head_type == DEREF) {

		sto = registerize(codegen(tree->child[0]->child[0]));
		sto2 = registerize(codegen(tree->child[1]));
		printf("movl %s, (%s)\n", sto2, sto);

		return sto2;
	}

	/* simple variable assignment */
	if (tree->head_type == ASGN && tree->child_count == 2
		&& tree->child[0]->head_type == VARIABLE) {
		sym = sym_lookup(tree->child[0]->tok);
		if (tree->child[1]->head_type == NUMBER) {
			/* optimized code for number operand */
			printf("movl $%s, %s\n",
				get_tok_str(*(tree->child[1]->tok)), symstack(sym));
			return symstack(sym);
		} else if (tree->child[1]->head_type == VARIABLE) {
			/* optimized code for variable operand */
			sto = symstack(sym_lookup(tree->child[1]->tok));
			sto2 = get_temp_reg();
			printf("movl %s, %s\n", sto, sto2);
			printf("movl %s, %s\n", sto2, symstack(sym));
			free_temp_reg(sto2);
			return symstack(sym);
		} else {
			/* general case */
			sto = codegen(tree->child[1]);
			sto2 = registerize(sto);
			printf("movl %s, %s\n", sto2, symstack(sym));
			free_temp_reg(sto2);
			return symstack(sym);
		}
	}

	/* array assignment */
	if (tree->head_type == ASGN && tree->child_count == 2
		&& tree->child[0]->head_type == ARRAY) {
		/* head address */
		sym = sym_lookup(tree->child[0]->child[0]->tok);
		/* index expression */
		str = codegen(tree->child[0]->child[1]);
		sto2 = registerize(str);
		/* right operand */
		str2 = codegen(tree->child[1]);
		sto3 = registerize(str2);

		printf("imull $4, %s\n", sto2);
		printf("addl %s, %s\n", symstack(sym), sto2);
		printf("movl %s, (%s)\n", sto3, sto2);

		free_temp_reg(sto2);
		free_temp_reg(sto3);

		return sto3;
	}

	/* array retrieval */
	if (tree->head_type == ARRAY && tree->child_count == 2) {
		/* head address */
		sym = sym_lookup(tree->child[0]->tok);
		/* index expression */
		printf("# index expr\n");
		str = codegen(tree->child[1]);
		sto2 = registerize(str);

		sto = get_temp_reg();
		printf("# build ptr\n");
		printf("imull $4, %s\n", sto2);
		printf("addl %s, %s\n", symstack(sym), sto2);
		printf("movl (%s), %s\n", sto2, sto);

		free_temp_reg(sto2);

		return sto;
	}

	/* number */
	if (tree->head_type == NUMBER) {
		sto = get_temp_reg();
		printf("movl $%s, %s\n", 
			get_tok_str(*(tree->tok)),
			sto);
		return sto;
	}

	/* variable */
	if (tree->head_type == VARIABLE) {
		sto = get_temp_reg();
		sym = sym_lookup(tree->tok);
		printf("movl %s, %s\n", 
			symstack(sym), sto);
		return sto;
	}

	/* arithmetic */
	if ((arith = arith_op(tree->head_type)) && tree->child_count) {
		/* (with optimized code for number and variable operands
		 * that avoids wasting temporary registes) */
		sto = get_temp_reg();
		for (i = 0; i < tree->child_count; i++) {
			oper = i ? arith : "movl";
			if (tree->child[i]->head_type == NUMBER) {
				printf("%s $%s, %s\n", 
					oper, get_tok_str(*(tree->child[i]->tok)), sto);
			} else if(tree->child[i]->head_type == VARIABLE) {
				sym = sym_lookup(tree->child[i]->tok);
				printf("%s %s, %s\n", oper, symstack(sym), sto);
			} else {
				str = codegen(tree->child[i]);
				printf("%s %s, %s\n", oper, str, sto);
				free_temp_reg(str);
			}
		}
		return sto;
	}


	/* division. division is special on x86.
	 * I hope I got this right.
	 *
	 * This also implements % because they are
	 * done using the same instruction on x86.
	 */
	if ((tree->head_type == DIV || tree->head_type == MOD)
		&& tree->child_count) {
		sto = get_temp_mem();
		sav1 = sav2 = NULL;
		/* 
		 * If EAX or EDX are in use, save them
		 * to temporary stack storage, restoring
		 * when the division tree is done
		 */
		if (ts_used[0]) {	/* EAX in use ? */
			sav1 = get_temp_mem();
			printf("movl %%eax, %s\n", sav1);
		}
		if (ts_used[3]) {	/* EDX in use ? */
			sav2 = get_temp_mem();
			printf("movl %%edx, %s\n", sav2);			
		}
		ts_used[0] = ts_used[3] = 1;
		/* code the dividend */
		str = codegen(tree->child[0]);
		/* put the 32-bit dividend in EAX */
		printf("movl %s, %%eax\n", str);
		/* clear EDX (higher 32 bits of 64-bit dividend) */
		printf("xor %%edx, %%edx\n");
		/* extend EAX sign to EDX */
		if (tree->head_type != MOD)
			printf("cdq\n");
		for (i = 1; i < tree->child_count; i++) {
			/* (can't idivl directly by a number, eh ?) */
			if(tree->child[i]->head_type == VARIABLE) {
				sym = sym_lookup(tree->child[i]->tok);
				printf("idivl %s\n", symstack(sym));
				if (tree->head_type != MOD) {
					printf("xor %%edx, %%edx\n");
					printf("cdq\n");
				}
			} else {
				str = codegen(tree->child[i]);
				printf("idivl %s\n", str);
				if (tree->head_type != MOD) {
					printf("xor %%edx, %%edx\n");
					printf("cdq\n");
				}
				free_temp_reg(str);
				free_temp_mem(str);
			}
		}
		/* move EAX to some temporary storage */
		printf("movl %s, %s\n", 
			tree->head_type == MOD ? "%edx" : "%eax",
			sto);
		/* restore or free EAX and EDX */
		if (sav1)
			printf("movl %s, %%eax\n", sav1);
		else
			free_temp_reg("%eax");
		if (sav2)
			printf("movl %s, %%edx\n", sav2);
		else
			free_temp_reg("%edx");
		return sto;
	}


	/* optimized code for if ( A < B) etc. */
	if (tree->head_type == IF
	&& tree->child[0]->head_type == LT)
		return optimized_if(tree, "jge");
	if (tree->head_type == IF
	&& tree->child[0]->head_type == LTE)
		return optimized_if(tree, "jg");
	if (tree->head_type == IF
	&& tree->child[0]->head_type == GT)
		return optimized_if(tree, "jle");
	if (tree->head_type == IF
	&& tree->child[0]->head_type == GTE)
		return optimized_if(tree, "jl");
	if (tree->head_type == IF
	&& tree->child[0]->head_type == NEQL)
		return optimized_if(tree, "je");
	if (tree->head_type == IF
	&& tree->child[0]->head_type == EQL)
		return optimized_if(tree, "jne");

	/* general-case if */
	if (tree->head_type == IF) {
		lab1 = intl_label++;
		lab2 = intl_label++;
		/* codegen the conditional */
		sto = codegen(tree->child[0]);
		/* branch if the conditional is false */
		str = registerize(sto);
		str2 = get_temp_reg();
		printf("movl $0, %s\n", str2);
		printf("cmpl %s, %s\n", str, str2);
		free_temp_reg(sto);
		free_temp_reg(str);
		free_temp_reg(str2);
		printf("je IL%d\n",	lab1);
		/* codegen "true" block */
		codegen(tree->child[1]);
		/* check for else-return pattern;
		 * if it is encountered, the else
		 * label and jump are not necessary */
		else_ret = tree->child_count == 3
		 && tree->child[2]->head_type == RET;
		/* jump over else block if there
		 * is one */
		if (tree->child_count == 3
			&& !else_ret)
			printf("jmp IL%d\n", lab2);
		printf("IL%d: \n", lab1);
		/* code the else block, if any */
		if (tree->child_count == 3)
			codegen(tree->child[2]);
		if (!else_ret)
			printf("IL%d: \n", lab2);
		return NULL;
	}

	/* optimized code for while ( A < B) etc. */
	if (tree->head_type == WHILE
	&& tree->child[0]->head_type == LT)
		return optimized_while(tree, "jge");
	if (tree->head_type == WHILE
	&& tree->child[0]->head_type == LTE)
		return optimized_while(tree, "jg");
	if (tree->head_type == WHILE
	&& tree->child[0]->head_type == GT)
		return optimized_while(tree, "jle");
	if (tree->head_type == WHILE
	&& tree->child[0]->head_type == GTE)
		return optimized_while(tree, "jl");
	if (tree->head_type == WHILE
	&& tree->child[0]->head_type == NEQL)
		return optimized_while(tree, "je");
	if (tree->head_type == WHILE
	&& tree->child[0]->head_type == EQL)
		return optimized_while(tree, "jne");

	/* general-case while */
	if (tree->head_type == WHILE) {
		lab1 = intl_label++;
		lab2 = intl_label++;
		/* codegen the conditional */
		printf("IL%d: \n", lab1);
		sto = codegen(tree->child[0]);
		/* branch if the conditional is false */
		str = registerize(sto);
		str2 = get_temp_reg();
		printf("movl $0, %s\n", str2);
		printf("cmpl %s, %s\n", str, str2);
		free_temp_reg(sto);
		free_temp_reg(str);
		free_temp_reg(str2);
		printf("je IL%d\n",	lab2);
		/* codegen the block */
		codegen(tree->child[1]);
		/* jump back to the conditional */
		printf("jmp IL%d\n", lab1);
		printf("IL%d: \n", lab2);
		return NULL;
	}

	/* negative sign */
	if (tree->head_type == NEGATIVE) {
		sto = get_temp_mem();
		sto2 = get_temp_reg();
		str = codegen(tree->child[0]);
		printf("movl $0, %s\n", sto2);
		printf("subl %s, %s\n", str, sto2);
		printf("movl %s, %s\n", sto2, sto);
		free_temp_reg(str);
		return sto;
	}

	/* goto */
	if (tree->head_type == GOTO) {
		printf("jmp %s\n", get_tok_str(*(tree->tok)));
		return NULL;
	}

	/* label */
	if (tree->head_type == LABEL) {
		printf("%s:\n", get_tok_str(*(tree->tok)));
		return NULL;
	}

	/* discarded tree */
	if (tree->head_type == NULL_TREE)
		return NULL;

	fprintf(stderr, "Sorry, I can't yet codegen this tree: \n");
	fflush(stderr);
	printout_tree(*tree);
	fputc('\n', stderr);
	exit(1);
}

/* 
 * Simple implementation of a generic relational operator.
 * Needs tree with the operands as children and the x86
 * jump mnemonic for the *opposite* check; for example, the
 * opposite for "less than" is "greater or equal" which
 * has jump mnemonic "jge".
 */
char* cheap_relational(exp_tree_t* tree, char *oppcheck)
{
		char *sto, *sto2, *sto3;
		char *str, *str2;
		sto3 = get_temp_reg();
		printf("movl $0, %s\n", sto3);
		sto = codegen(tree->child[0]);
		str = get_temp_reg();
		printf("movl %s, %s\n", sto, str);
		free_temp_reg(sto);
		sto2 = codegen(tree->child[1]);
		str2 = get_temp_reg();
		printf("movl %s, %s\n", sto2, str2);
		free_temp_reg(sto2);
		printf("cmpl %s, %s\n", str2, str);
		free_temp_reg(str);
		free_temp_reg(str2);
		printf("%s IL%d\n", oppcheck, intl_label);
		printf("movl $1, %s\n", sto3);
		printf("IL%d: \n", intl_label++);
		free_temp_reg(sto);
		free_temp_reg(sto2);
		return sto3;
}

/* 
 * Write optimized code for cases like
 *	if ( A < B ), if (A == B), etc.
 * Again, the tree and the jump mnemonic
 * for the opposite check is needed.
 */
char* optimized_if(exp_tree_t* tree, char *oppcheck)
{
	char *str, *str2, *sto, *sto2;
	int lab1 = intl_label++;
	int lab2 = intl_label++;
	/* optimized conditonal */
	str = codegen(tree->child[0]->child[0]);
	str2 = codegen(tree->child[0]->child[1]);
	sto = registerize(str);
	sto2 = registerize(str2);
	printf("cmpl %s, %s\n", sto2, sto);
	free_temp_reg(sto);
	free_temp_reg(sto2);
	printf("%s IL%d\n", oppcheck, lab1);
	/* codegen "true" block */
	codegen(tree->child[1]);
	/* check for else-return pattern;
	 * if it is encountered, the else
	 * label and jump are not necessary */
	else_ret = tree->child_count == 3
	 && tree->child[2]->head_type == RET;
	/* jump over else block if there
	 * is one */
	if (tree->child_count == 3
		&& !else_ret)
		printf("jmp IL%d\n", lab2);
	printf("IL%d: \n", lab1);
	/* code the else block, if any */
	if (tree->child_count == 3)
		codegen(tree->child[2]);
	if (!else_ret)
		printf("IL%d: \n", lab2);
	return NULL;
}

/* Same salad as above, but for while loops */
char* optimized_while(exp_tree_t* tree, char *oppcheck)
{
	char *str, *str2, *sto, *sto2;
	int lab1 = intl_label++;
	int lab2 = intl_label++;
	/* optimized conditional branch */
	printf("IL%d: \n", lab1);
	str = codegen(tree->child[0]->child[0]);
	str2 = codegen(tree->child[0]->child[1]);
	sto = registerize(str);
	sto2 = registerize(str2);
	printf("cmpl %s, %s\n", sto2, sto);
	free_temp_reg(sto);
	free_temp_reg(sto2);
	printf("%s IL%d\n",	oppcheck, lab2);
	/* codegen the block */
	codegen(tree->child[1]);
	/* jump back to the conditional */
	printf("jmp IL%d\n", lab1);
	printf("IL%d: \n", lab2);
	return NULL;
}


