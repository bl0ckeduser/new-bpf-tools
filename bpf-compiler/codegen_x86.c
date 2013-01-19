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
char ts_used[TEMP_REGISTERS];
char tm_used[TEMP_MEM];
int syms = 0;
int arg_syms = 0;
int temp_register = 0;
int swap = 0;
int proc_ok = 1;
int else_ret;

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
		case DIV:
			return "idivl";
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

	/* setup the identifier symbol table and make stack
	 * space for all the variables in the program */
	setup_symbols(tree);

	for (i = 0; i < TEMP_MEM; ++i) {
		buf2 = malloc(64);
		strcpy(buf2, symstack(nameless_perm_storage()));
		temp_mem[i] = buf2;
	}

	/* make the symbol and label for the procedure */
#ifndef MINGW_BUILD
	printf(".type %s, @function\n", name);
#endif
	printf("%s:\n", name);

	/* do the usual x86 function entry process,
	 * and set aside stack memory for local
	 * variables */
	printf("# set up stack space\n");
	printf("pushl %%ebp\n");
	printf("movl %%esp, %%ebp\n");
	printf("subl $%d, %%esp\n", syms * 4);
	printf("\n# >>> compiled code for '%s'\n", name); 

	printf("_tco_%s:\n", name);	/* hook for TCO */

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
#ifdef MINGW_BUILD
	char main_name[] = "_main";
#else
	char main_name[] = "main";
#endif

	printf(".section .rodata\n");
	printf("_echo_format: .string \"%%d\\n\"\n");
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
		/* make a symbol named after the array
		 * at its index 0 */
		if (!sym_check(tree->child[0]->tok))
			(void)sym_add(tree->child[0]->tok);
		/* make nameless storage for the subsequent
		 * indices -- when we deal with an expression
		 * such as array[index] we only need to know
		 * the starting point of the array and the
		 * value "index" evaluates to */
		sto = atoi(get_tok_str(*(tree->child[1]->tok)));
		for (i = 0; i < sto - 1; i++)
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
	char *sto, *sto2, *sto3;
	char *str, *str2;
	char *name;
	char *proc_args[32];
	char my_ts_used[TEMP_REGISTERS];
	char *buf;
	int i;
	int arg_count;
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

	if (tree->head_type == BLOCK
		|| tree->head_type == IF
		|| tree->head_type == WHILE
		|| tree->head_type == BPF_INSTR) {
		/* clear temporary memory and registers */
		new_temp_mem();
		new_temp_reg();
	}

	/* procedure call */
	if (tree->head_type == PROC_CALL) {
		arg_count = tree->child_count;

		/* push all the registers being used */
		for (i = 0; i < TEMP_REGISTERS; ++i)
			if (ts_used[i])
				printf("pushl %s\t# save temp register\n",
					temp_reg[i]);
		memcpy(my_ts_used, ts_used, TEMP_REGISTERS);

		/* push the arguments in reverse order */
		for (i = arg_count - 1; i >= 0; --i) {
			sto = codegen(tree->child[i]);
			printf("pushl %s\t# argument %d to %s\n", sto, i,
				 get_tok_str(*(tree->tok)));
			free_temp_reg(sto);
		}

		/* call that routine */
		printf("call %s\n", get_tok_str(*(tree->tok)));

		/* throw off the arguments from the stack */
		printf("addl $%d, %%esp\t# throw off %d arg%s\n",
			4 * arg_count, arg_count,
			arg_count > 1 ? "s" : "");

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

		/* count the arguments */
		arg_count = 0;
		for (i = 0; tree->child[0]->child[i]; ++i)
			++arg_count;

		/* move the arguments directly to the
		 * callee stack offsets */
		for (i = arg_count - 1; i >= 0; --i) {
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
		/* build pointer */
		sto = get_temp_reg();
		printf("movl %s, %s\n",
			str, sto);
		printf("imull $-4, %s\n",
			sto);
		printf("addl %%ebp, %s\n",
			sto);
		printf("subl $%d, %s\n",
			4 * sym, sto);
		/* write the final move */
		sto2 = get_temp_reg();
		printf("incl (%s)\n", sto);
		printf("movl (%s), %s\n", sto, sto2);
		free_temp_reg(sto);
		return sto2;
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
		/* right operand */
		str2 = codegen(tree->child[1]);
		sto3 = registerize(str2);
		/* build pointer */
		sto2 = registerize(str);
		printf("imull $-4, %s\n",
			sto2);
		printf("addl %%ebp, %s\n",
			sto2);
		printf("subl $%d, %s\n",
			4 * sym, sto2);
		/* write the final move */
		sto = get_temp_mem();
		printf("movl %s, (%s)\n",
			sto3, sto2);
		free_temp_reg(sto2);
		free_temp_reg(sto3);
		return str2;
	}

	/* array retrieval */
	if (tree->head_type == ARRAY && tree->child_count == 2) {
		/* head address */
		sym = sym_lookup(tree->child[0]->tok);
		/* index expression */
		str = codegen(tree->child[1]);
		/* build pointer */
		sto2 = registerize(str);
		printf("imull $-4, %s\n",
			sto2);
		printf("addl %%ebp, %s\n",
			sto2);
		printf("subl $%d, %s\n",
			4 * sym, sto2);
		/* write the final move */
		sto = get_temp_reg();
		printf("movl (%s), %s\n",
			sto2, sto);
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


