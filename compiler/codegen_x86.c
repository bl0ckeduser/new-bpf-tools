/* 
 * Syntax tree ==> 32-bit x86 assembly (GAS syntax) generator 
 * 
 * It's my first time (quickly) doing x86 so it's probably far
 * from optimal. The essence of this module is juggling with
 * registers and coping with permitted parameter-types. 
 * Many improvements are possible.
 *
 * Note that this code generator uses the standard C boolean
 * idiom of 0 = false, nonzero = true, unlike the BPFVM one.
 */

/* ====================================================== */

#include "tree.h"
#include "tokens.h"
#include "typedesc.h"
#include "codegen_x86.h"
#include <string.h>
#include <stdio.h>

extern void fail(char*);
extern void compiler_fail(char *message, token_t *token,
	int in_line, int in_chr);
extern void compiler_warn(char *message, token_t *token,
	int in_line, int in_chr);
extern token_t *findtok(exp_tree_t *et);

void setup_symbols(exp_tree_t* tree, int glob);

/* ====================================================== */

/*
 * Modes for the symbol setup
 * routine which populates the
 * various symbol tables and
 * which sets aside memory
 */
enum {
	/* symbol is local to a function */
	SYMTYPE_STACK,

	/* symbol is a global */
	SYMTYPE_GLOBALS
};

/* Name of function currently being coded */
char current_proc[64];

/* Table of local symbols */
char symtab[256][32] = {""};
int syms = 0;			/* count */
int symbytes = 0;		/* stack size in bytes */
int symsiz[256] = {0};	/* size of each object */
typedesc_t symtyp[256];

/* Table of global symbols */
char globtab[256][32] = {""};
int globs = 0;	/* count */
typedesc_t globtyp[256];

/* Table of function-argument symbols */
char arg_symtab[256][32] = {""};
int arg_syms = 0;		/* count */
int argbytes = 0;		/* total size in bytes */
int argsiz[256] = {0};	/* size of each object */
typedesc_t argtyp[256];

/* Table of string-constant symbols */
char str_const_tab[256][32] = {""};
int str_consts = 0;

/* Temporary-use registers currently in use */
char ts_used[TEMP_REGISTERS];

/* Temporary-use stack offsets currently in use */
char tm_used[TEMP_MEM];

int proc_ok = 1;
int else_ret;			/* used for "else return" optimization */
int main_defined = 0;	/* set if user-defined main() exists */
int ccid = 0;

int stack_size;
int intl_label = 0; /* internal label numbering */

char entry_buf[1024], buf[1024];

/* ====================================================== */

void codegen_fail(char *msg, token_t *tok)
{
	compiler_fail(msg, tok, 0, 0);
}

/* 
 * INT_DECL => 4 because "int" is 4 bytes, etc.
 * (this program compiles code specifically 
 * for 32-bit x86, so I can say that 
 * "int" is 4 bytes with no fear of being 
 * crucified by pedants).
 */
int decl2siz(int bt)
{
	switch (bt) {
		case INT_DECL:
			return 4;
		case CHAR_DECL:
			return 1;
		default:
			return 0;
	}
}

/* 
 * Type description => size of datum in bytes
 */
int type2siz(typedesc_t ty)
{
	/* 
	 * Special case for "Negative-depth" pointers in 
	 * nincompoop situations like *3 
	 */
	if (ty.ptr < 0 || ty.arr < 0)
		return 0;

	/* 
	 * If it's any kind of pointer, the size is 
	 * 4 bytes (32-bit word), otherwise it's the size 
 	 * of the base type
	 */
	return ty.ptr || ty.arr ? 4 : decl2siz(ty.ty);
}

/*
 * This compiler uses GNU i386 assembler (or compatible), invoked
 * via "gcc" or "clang" for assembling.
 *
 * GNU i386 assembler version 2.22 gives messages like this:
 * "Warning: using `%bl' instead of `%ebx' due to `b' suffix"
 * when it sees code like "movb %ebx, (%eax)", but it compiles
 * without any further issues.
 *
 * GNU i386 assembler version 2.17.50 gives the diagnostic
 * "error: invalid operand for instruction" for the same code and
 * failes to compile.
 *
 * So here is a quick fix.
 */
char *fixreg(char *r, int siz)
{
	char *new;
	if (siz == 1) {
		if (strlen(r) == 4 && r[0] == '%'
			&& r[1] == 'e' && r[3] == 'x') {
			new = malloc(64);
			sprintf(new, "%%%cl", r[2]);
			return new;
		}
	}
	return r;
}

/*
 * Generate the GNU x86 MOV instruction
 * for reading a `membsiz'-sized integer
 * datum into 32-bit word.
 * 
 * I found out about this by looking
 * at "gcc -S" x86 output.
 *
 * This is used for example to retrieve
 * the contents of a 1-byte `char' variable 
 * into a 32-bit word register like EAX.
 *
 * XXX: I don't know if it's signed
 * or unsigned or whatever for bytes
 */
char *move_conv_to_long(int membsiz)
{
	switch (membsiz) {
		case 1:
			return "movsbl";
		case 4:
			return "movl";
		default:
			fail("type conversion codegen fail");
	}
}

/* 
 * Build a type-description structure
 */
typedesc_t mk_typedesc(int bt, int ptr, int arr)
{
	typedesc_t td;
	td.ty = bt;
	td.ptr = ptr;
	td.arr = arr;
	td.arr_dim = NULL;
	return td;
}

/* 
 * This is required by main(), because
 * it is used in the BPFVM codegen
 * which was written before this one.
 */
void print_code() {
	;
}

/* 
 * Release all temporary registers
 * for new use
 */
void new_temp_reg() {
	int i;
	for (i = 0; i < TEMP_REGISTERS; ++i)
		ts_used[i] = 0;
}

/* 
 * Get a general-purpose X86 register for
 * temporary use 
 */
char* get_temp_reg() {
	int i;
	for (i = 0; i < TEMP_REGISTERS; ++i)
		if (!ts_used[i]) {
			ts_used[i] = 1;
			return temp_reg[i];
		}
	fail("out of registers");
}

/* 
 * Explicitly let go of a temporary register 
 */
void free_temp_reg(char *reg) {
	int i;

	for (i = 0; i < TEMP_REGISTERS; ++i)
		if (!strcmp(reg, temp_reg[i]))
			ts_used[i] = 0;
}

/* 
 * Release all temporary stack-based
 * storage for new use
 */
void new_temp_mem() {
	int i;
	for (i = 0; i < TEMP_MEM; ++i)
		tm_used[i] = 0;
}

/* 
 * Get some temporary stack storage
 */
char* get_temp_mem() {
	int i;
	for (i = 0; i < TEMP_MEM; ++i)
		if (!tm_used[i]) {
			tm_used[i] = 1;
			return temp_mem[i];
		}
	fail("out of temporary memory");
}

/* 
 * Explicitly let go of temporary stack memory
 */
void free_temp_mem(char *reg) {
	int i;

	for (i = 0; i < TEMP_MEM; ++i)
		if (!strcmp(reg, temp_mem[i]))
			tm_used[i] = 0;
}

/* 
 * Get a GNU x86 assembler syntax string
 * for a stack byte offset
 */
char *symstack(int id) {
	static char buf[128];
	if (!id)
		sprintf(buf, "(%%ebp)");
	else if (id < 0)
		sprintf(buf, "%d(%%ebp)", -id /* * 4 */);
	else
		sprintf(buf, "-%d(%%ebp)", id /* * 4 */);
	return buf;
}

/* 
 * Extract the raw string from a token 
 */
char* get_tok_str(token_t t)
{
	static char buf[1024];
	strncpy(buf, t.start, t.len);
	buf[t.len] = 0;
	return buf;
}


/* 
 * Check if a global is already defined 
 */
int glob_check(token_t* tok)
{
	int i;
	char *s = get_tok_str(*tok);
	char buf[128];

	for (i = 0; i < globs; i++)
		if (!strcmp(globtab[i], s)) {
			sprintf(buf, "global symbol `%s' defined twice", s);
			compiler_fail(buf, tok, 0, 0);
		}
	return 0;
}

/* 
 * Check if a local (stack) symbol is 
 * already defined 
 */
int sym_check(token_t* tok)
{
	int i;
	char *s = get_tok_str(*tok);
	char buf[128];

	for (i = 0; i < syms; i++)
		if (!strcmp(symtab[i], s)) {
			sprintf(buf, "local symbol `%s' defined twice", s);
			compiler_fail(buf, tok, 0, 0);
		}

	for (i = 0; i < arg_syms; i++)
		if (!strcmp(arg_symtab[i], s)) {
			sprintf(buf, "declaration of local symbol `%s' conflicts"
						 " with an argument having the same name", s);
			compiler_fail(buf, tok, 0, 0);
		}
	return 0;
}

/*
 * Allocate a permanent storage register without
 * giving it a symbol name. Useful for special 
 * registers created by the compiler. 
 */
char* nameless_perm_storage(int siz)
{
	/* 
	 * It's important to clear this in case
	 * there is some leftover stuff 
	 */
	*(symtab[syms]) = 0;

	symsiz[syms++] = siz;

	return symstack((symbytes += siz) - siz);
}

/*
 * Create a new symbol, obtaining its permanent
 * storage address.
 */ 
int sym_add(token_t *tok, int size)
{
	char *s = get_tok_str(*tok);
	strcpy(symtab[syms], s);
	symsiz[syms] = size;
	symbytes += size;
	return syms++; 
}

/*
 * Create a new global
 */ 
int glob_add(token_t *tok)
{
	char *s = get_tok_str(*tok);
	strcpy(globtab[globs], s);
	return globs++; 
}

/*
 * Copy a string to new memory
 */
char *newstr(char *s)
{
	char *new = malloc(strlen(s) + 1);
	strcpy(new, s);
	return new;
}

/*
 * Compute the address offset of 
 * an object in memory containing
 * mixed-sized objects
 */
int offs(int *sizes, int ndx)
{
	int i;
	int o = 0;
	for (i = 0; i < ndx; ++i)
		o += sizes[i];
	return o;
}

/* 
 * Find the GAS syntax for the address of 
 * a symbol, which could be a local stack
 * symbol, a global, or a function argument
 * (which is also on the stack, but in a
 * a funny position).
 */
char* sym_lookup(token_t* tok)
{
	char buf[1024];
	char *s = get_tok_str(*tok);
	int i = 0;

	/* Try stack locals */
	for (i = 0; i < syms; i++)
		if (!strcmp(symtab[i], s)) {
			return newstr(symstack(offs(symsiz, i)));
		}

	/* Try arguments */
	for (i = 0; i < arg_syms; i++)
		if (!strcmp(arg_symtab[i], s))
			return newstr(symstack(-2*4 - offs(argsiz, i)));

	/* 
	 * Try globals last -- they are shadowed
	 * by locals and arguments 
	 */
	for (i = 0; i < globs; ++i)
		if (!strcmp(globtab[i], s))
			return globtab[i];

	sprintf(buf, "unknown symbol `%s'", s);
	compiler_fail(buf, tok, 0, 0);
}

/* 
 * Lookup a symbol and return
 * information about its type
 */
typedesc_t sym_lookup_type(token_t* tok)
{
	char buf[1024];
	char *s = get_tok_str(*tok);
	int i = 0;

	/* Try stack locals */
	for (i = 0; i < syms; i++)
		if (!strcmp(symtab[i], s))
			return symtyp[i];

	/* Try arguments */
	for (i = 0; i < arg_syms; i++)
		if (!strcmp(arg_symtab[i], s))
			return argtyp[i];

	/* 
	 * Try globals last -- they are shadowed
	 * by locals and arguments 
	 */
	for (i = 0; i < globs; ++i)
		if (!strcmp(globtab[i], s))
			return globtyp[i];

	sprintf(buf, "unknown symbol `%s'", s);
	compiler_fail(buf, tok, 0, 0);
}

/* 
 * Lookup storage number of a string constant
 */
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

/* 
 * Add a string constant to the string constant table
 */
int str_const_add(token_t *tok)
{
	char *s = get_tok_str(*tok);
	strcpy(str_const_tab[str_consts], s);
	return str_consts++; 
}

/* 
 * Tree type -> arithmetic routine
 * (this is for the easy general
 * cases -- divison and remainders
 * need special attention on X86)
 */
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
	char buf[1024];
	char *buf2;
	int i;

	/* If syms is set to 0, funny things happen */
	syms = 1;
	symbytes = 4;
	symsiz[0] = 4;
	arg_syms = 0;

	strcpy(current_proc, name);

	/* Copy the argument symbols to the symbol table */
	for (i = 0; args[i]; ++i) {
		strcpy(arg_symtab[arg_syms], args[i]);
		/* XXX: assumes int args */
		argtyp[arg_syms].ty = INT_DECL;
		argtyp[arg_syms].arr = 0;
		argtyp[arg_syms].ptr = 0;
		argsiz[arg_syms] = 4;
		argbytes += 4;
		++arg_syms;
	}

	/* Make the symbol and label for the procedure */
#ifndef MINGW_BUILD
	printf(".type %s, @function\n", name);
#endif
	printf("%s:\n", name);

	/* 
	 * Populate the symbol table with the
	 * function's local variables and set aside
	 * stack space for them
	 */
	*entry_buf = 0;
	setup_symbols(tree, SYMTYPE_STACK);

	for (i = 0; i < TEMP_MEM; ++i) {
		buf2 = malloc(64);
		strcpy(buf2, nameless_perm_storage(4));
		temp_mem[i] = buf2;
	}

	/* 
	 * Do the usual x86 function entry process,
	 * moving down the stack pointer to make
	 * space for the local variables 
	 */
	printf("# set up stack space\n");
	printf("pushl %%ebp\n");
	printf("movl %%esp, %%ebp\n");
	printf("subl $%d, %%esp\n", symbytes);
	printf("\n# >>> compiled code for '%s'\n", name);

	printf("_tco_%s:\n", name);	/* hook for TCO */
	puts(entry_buf);

	/* 
	 * Code the body of the procedure
	 */
	codegen(tree);

	/* Do a typical x86 function return */
	printf("\n# clean up stack and return\n");
	printf("_ret_%s:\n", name);	/* hook for 'return' */
	printf("addl $%d, %%esp\n", symbytes);
	printf("movl %%ebp, %%esp\n");
	printf("popl %%ebp\n");
	printf("ret\n\n");
}

/*
 * Determine if main() is defined
 * in a user's program.
 */
int look_for_main(exp_tree_t *tree)
{
	int i;

	if (tree->head_type == PROC) {
		if (!strcmp(get_tok_str(*(tree->tok)),
					"main"))
			return 1;
	}

	/* child-recursion */
	for (i = 0; i < tree->child_count; ++i)
		if (tree->child[i]->head_type == BLOCK
		|| tree->child[i]->head_type == PROC)
			if (look_for_main(tree->child[i]))
				return 1;

	return 0;
}

/* 
 * Entry point of the x86 codegen
 */
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

	/*
	 * Define the format used for printf()
	 * in the echo() code, then code the
	 * user's string constants
	 */
	printf(".section .rodata\n");
	printf("# start string constants ========\n");
	printf("_echo_format: .string \"%%d\\n\"\n");
	deal_with_str_consts(tree);
	printf("# end string constants ==========\n\n");

	/*
	 * Find out if the user defined main()
	 */
	main_defined = look_for_main(tree);

	/*
	 * If there is a main() defined,
	 * gotta do globals
	 */
	if (main_defined) {
		printf("# start globals =============\n");
		printf(".section .data\n");
		setup_symbols(tree, SYMTYPE_GLOBALS);
		printf(".section .text\n");
		printf("# end globals =============\n\n");
	}

	/* Don't ask me why this is necessary */
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

	/*
	 * Code a builtin echo(int n) utility routine
	 */
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

	/*
	 * Test/debug the type analyzer
	 */
	#ifdef DEBUG
		int i;
		exp_tree_t *child;
		for (i = 0; i < tree->child_count; ++i)
			printout_tree(*(tree->child[i])),
			 fprintf(stderr, "\n"),
			 dump_td(tree_typeof(tree->child[i]));
	#endif
}

/*
 * Compile string constants
 */
void deal_with_str_consts(exp_tree_t *tree)
{
	int i;
	int id;

	if (tree->head_type == STR_CONST) {
		id = str_const_add(tree->tok);
		/* 
		 * The quotation marks are part of the
		 * token string, so there is no need 
		 * to write them again.
		 */
		printf("_str_const_%d: .string %s\n", 
			id, get_tok_str(*(tree->tok)));

		/* XXX: TODO: escape sequences like \n ... */
	}

	/* child recursion */
	for (i = 0; i < tree->child_count; ++i)
		deal_with_str_consts(tree->child[i]);
}


/* 
 * Code all the function definitions in a program
 */
void deal_with_procs(exp_tree_t *tree)
{
	int i;

	if (tree->head_type == PROC) {
		syms = 0;
		arg_syms = 0;
		codegen(tree);
		(*tree).head_type = NULL_TREE;
	}

	/* child recursion */
	for (i = 0; i < tree->child_count; ++i)
		if (tree->child[i]->head_type == BLOCK
		|| tree->child[i]->head_type == PROC)
			deal_with_procs(tree->child[i]);
}

/* 
 * Check if a declaration is for an array,
 * and if it is, return dimensions 
 */
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
 * Get a GAS X86 instruction suffix
 * from a type declaration code
 */
char *decl2suffix(char ty)
{
	switch (ty) {
		case INT_DECL:
			return "l";
		case CHAR_DECL:
			return "b";
		default:
			return "";
	}
}

/*
 * 		Byte size of an integer object
 * =>	GAS instruction suffix
 */
char *siz2suffix(char siz)
{
	switch (siz) {
		case 4:
			return "l";
		case 1:
			return "b";
		default:
			return "";
	}
}

/* 
 * Check for integer-type type declaration codes
 */
int int_type_decl(char ty)
{
	return ty == CHAR_DECL || ty == INT_DECL;
}

/* See codegen_proc above */
void setup_symbols(exp_tree_t *tree, int symty)
{
	int i, j, k, sto;
	exp_tree_t *dc;
	int stars, newlen;
	char *str;
	int array_ptr;
	int sym_num;
	int membsiz;
	int start_bytes;
	int decl = tree->head_type;
	char *suffx = decl2suffix(decl);

	/* Only integer (i.e. char, int) types are supported */
	if (int_type_decl(decl)) {
		for (i = 0; i < tree->child_count; ++i) {
			dc = tree->child[i];

			/* 
			 * Count & eat up the pointer-qualification stars,
			 * as in e.g. "int ***herp_derp"
		 	 */
			stars = newlen = 0;
			for (j = 0; j < dc->child_count; ++j)
				if (dc->child[j]->head_type == DECL_STAR)
					newlen = newlen ? newlen : j,
					++stars,
					dc->child[j]->head_type = NULL_TREE;
			if (stars)
				dc->child_count = newlen;

			/* 
			 * Figure out size of array members, in bytes
			 */
			membsiz = type2siz(mk_typedesc(decl, stars, check_array(dc)));

			/* Add some padding; otherwise werid glitches happen */
			if (membsiz < 4 && !check_array(dc))
				membsiz = 4;
	
			/* 
			 * Switch symbols/stack setup code
			 * based on array dimensions of object
			 */
			if (check_array(dc) > 1) {
				codegen_fail("N-dimensional, where N > 1, arrays are "
					         "currently unsupported", dc->child[0]->tok);
			} else if (check_array(dc) == 1) {
				/*
				 * Set up symbols and stack storage
			 	 * for a 1-dimensional array
				 */

				/* XXX: global arrays unsupported */
				if (symty == SYMTYPE_GLOBALS)
					codegen_fail("global arrays are currently unsupported",
						dc->child[0]->tok);

				/* Get number of elements (constant integer) */
				/* XXX: check it's actually an integer */
				sto = atoi(get_tok_str(*(dc->child[1]->tok)));

				/* 
				 * Make a symbol named after the array
				 * and store the *ADDRESS* of its first 
				 * element there 
				 */
				start_bytes = symbytes;
				if (!sym_check(dc->child[0]->tok)) {
					/* 
					 * 32-bit pointers must be always be 
					 * 4 bytes, not `membsiz', hence the 4
					 */
					sym_num = array_ptr = sym_add(dc->child[0]->tok, 4);
				}
				str = get_temp_reg();
				sprintf(buf, "leal %s, %s\n",
					symstack(start_bytes + sto * membsiz),
					str);
				strcat(entry_buf, buf);
				sprintf(buf, "movl %s, %s\n",
					str,
					symstack(start_bytes));
				strcat(entry_buf, buf);
				/* 
				 * Make storage for the subsequent
				 * indices -- (when we deal with an expression
				 * such as array[index] we only need to know
				 * the starting point of the array and the
				 * value "index" evaluates to)
				 */
				symsiz[syms++] = membsiz * sto;
				symbytes += membsiz * sto;

				/* Discard the tree */
				dc->head_type = NULL_TREE;
			} else {
				/*
				 * Set up symbols and stack storage
			 	 * for a plain variable (e.g. "int x")
				 */

				switch (symty) {
					case SYMTYPE_STACK:
						/* 
						 * Stack-based storage and symbol
						 */
						if (!sym_check(dc->child[0]->tok))
							sym_num = sym_add(dc->child[0]->tok, membsiz);
						break;
					case SYMTYPE_GLOBALS:
						/*
						 * Global / heap storage and symbol
						 */
						if (!glob_check(dc->child[0]->tok)) {
							/*
							 * If there is an initializer, make sure
							 * it is an integer constant.
							 */
							if (dc->child_count == 2
								&& dc->child[1]->head_type != NUMBER) {
								codegen_fail("non-integer-constant global initializer",
									dc->child[0]->tok);
							}

							/* 
							 * Make the symbol and write out the label
							 */
							sym_num = glob_add(dc->child[0]->tok);
							printf("%s: ",
								get_tok_str(*(dc->child[0]->tok)));

							/* 
							 * If no initial value is given,
							 * write a zero, otherwise write
							 * the initial value.
							 */
							/* XXX: change .long for types other than `int' */
							printf(".long %s\n.globl x\n\n",
								dc->child_count == 2 ?
									get_tok_str(*(dc->child[1]->tok))
									: "0");
						}
						break;
					default:
						fail("x86_codegen internal messup");
				}

				/* 
				 * For stack variables, make the initial value
				 * an assignment in the body of the relevant
				 * procedure; otherwise, discard the tree 
				 */
				if (dc->child_count == 2 
					&& symty == SYMTYPE_STACK)
					(*dc).head_type = ASGN;
				else
					(*dc).head_type = NULL_TREE;
			}

			/*
			 * Store information about the type of the object
		 	 */
			if (symty == SYMTYPE_STACK) {				
				symtyp[sym_num] = mk_typedesc(decl, stars, check_array(dc));
			} else if (symty == SYMTYPE_GLOBALS) {
				globtyp[sym_num] = mk_typedesc(decl, stars, check_array(dc));
			}
		}

		/* 
		 * Turn the declaration sequence into
		 * a standard code sequence, because at
		 * this point the assignments are ASGN
		 * and the rest are NULL_TREE
		 */
		tree->head_type = BLOCK;
	}
	
	/* Child recursion */
	for (i = 0; i < tree->child_count; ++i)
		if (tree->child[i]->head_type == BLOCK
		||	tree->child[i]->head_type == IF
		||	tree->child[i]->head_type == WHILE
		||	int_type_decl(tree->child[i]->head_type))
			setup_symbols(tree->child[i], symty);
}

/*
 * If a temporary storage location is already
 * a register, give it back as is. Otherwise,
 * copy it to a new temporary register and give
 * this new register. (This is used because
 * several x86 instructions seem to expect
 * registers rather than stack offsets)
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
	char sbuf[64];
	int i;
	char *sym_s;
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
	int ptr_arith_mode, obj_siz, ptr_memb, ptr_count;
	int membsiz;
	typedesc_t typedat;

	if (tree->head_type == BLOCK
		|| tree->head_type == IF
		|| tree->head_type == WHILE
		|| tree->head_type == BPF_INSTR) {
		/* clear temporary memory and registers */
		new_temp_mem();
		new_temp_reg();
	}

	/* 
	 * XXX: CAST tree -- might want to have some code here,
	 * e.g. for int -> char conversion or whatever,
	 * if that ever gets implemented 
	 */
	/* (CAST (CAST_TYPE (ty (INT_DECL))) (NUMBER:3)) */
	/* (CAST (CAST_TYPE (ty (INT_DECL)) (DECL_STAR)) (VARIABLE:p)) */
	if (tree->head_type == CAST) {
		return codegen(tree->child[1]);
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
				sym_lookup(tree->child[0]->tok),
				sto);

		return sto;
	}

	/* &(a[v]) = a + membsiz * v 
	 * -- this works for `a' of type int or char,
	 * at least assuming the index expression cleanly
	 * compiles to an int. note that it is legal for
	 * all the instructions to have `l' suffix because
	 * we're dealing with pointers.
	 */
	if (tree->head_type == ADDR
		&& tree->child_count == 1
		&& tree->child[0]->head_type == ARRAY) {

		membsiz = decl2siz(sym_lookup_type(tree->child[0]->child[0]->tok).ty);

		sto = get_temp_reg();

		/* load base address */
		printf("movl %s, %s\n",
				sym_lookup(tree->child[0]->child[0]->tok),
				sto);

		/* 
		 * now we code the array index expression.
		 * codegen() should convert
		 * whatever it encounters to `int',
		 * so no type-conversion worries
		 * if the array index is not an `int'
		 */
		sto2 = registerize(codegen(tree->child[0]->child[1]));


		/* multiply index byte-offset by size of members */
		printf("imull $%d, %s\n", membsiz, sto2);

		/* ptr = base_adr + membsiz * index_expr */
		printf("addl %s, %s\n", sto2, sto);

		return sto;
	}

	/* *(exp) */
	if (tree->head_type == DEREF
		&& tree->child_count == 1) {
		
		sto = registerize(codegen(tree->child[0]));
		sto2 = get_temp_reg();

		/* 
		 * Find byte size of *exp
	     */
		membsiz = type2siz(
			deref_typeof(tree_typeof(tree->child[0])));

		/* Check for crazy wack situations like *3 */
		if (membsiz <= 0) {
			membsiz = 1;
			compiler_warn("you're dereferencing what doesn't"
						  " seem to be a pointer.\nsegfaults might"
						  " be coming your way soon.",
						  findtok(tree), 0, 0);
		}

		/* use appropriate size suffix in the MOV */
		printf("mov%s (%s), %s\n", 
			siz2suffix(membsiz),
			sto, fixreg(sto2, membsiz));
		free_temp_reg(sto);

		return sto2;
	}

	/* procedure call */
	if (tree->head_type == PROC_CALL) {
		/* 
		 * Push all the temporary registers
		 * that are currently in use
		 */
		for (i = 0; i < TEMP_REGISTERS; ++i)
			if (ts_used[i])
				printf("pushl %s\t# save temp register\n",
					temp_reg[i]);
		memcpy(my_ts_used, ts_used, TEMP_REGISTERS);

		/* 
		 * Push the arguments in reverse order
		 */
		for (i = tree->child_count - 1; i >= 0; --i) {
			sto = codegen(tree->child[i]);
			/* 
			 * XXX: need to convert args to function type,
			 * e.g. char argument becomes int because prototype
			 * of function needs int. note that int is the
			 * default type in C.
			 */
  
			/* XXX: pushl is only for int args */
			printf("pushl %s\t# argument %d to %s\n", sto, i,
				 get_tok_str(*(tree->tok)));
			free_temp_reg(sto);
		}

		/* 
		 * Call the subroutine
		 */
		printf("call %s\n", get_tok_str(*(tree->tok)));

		/* 
		 * Throw off the arguments from the stack
		 */
		/* 
		 * XXX: FIXME: 4 * tree... assumes int args.
		 * must instead know the size in bytes of the
		 * whole argument stack
		 */
		printf("addl $%d, %%esp\t# throw off %d arg%s\n",
			4 * tree->child_count, tree->child_count,
			tree->child_count > 1 ? "s" : "");

		/* 
		 * Move the return-value register (EAX)
		 * to temporary stack-based storage
		 */
		sto = get_temp_mem();
		/* 
		 * XXX: what if the return type isn't int ?
		 * i don't even know what the convention would
		 * be for that.
		 * 
		 * int is the default return type in C.
		 */
		printf("movl %%eax, %s\n", sto);

		/* 
		 * Restore the registers as they were
		 * prior to the call -- pop them in 
		 * reverse order they were pushed
		 */
		for (i = 0; i < TEMP_REGISTERS; ++i)
			if (my_ts_used[TEMP_REGISTERS - (i + 1)])
				printf("popl %s\t# restore temp register\n",
					temp_reg[TEMP_REGISTERS - (i + 1)]);

		/* 
		 * Give back the temporary stack storage
		 * with the return value in it
		 */
		return sto;
	}

	/* procedure definition */
	if (tree->head_type == PROC) {
		if (!proc_ok)
			compiler_fail("proc not allowed here", tree->tok, 0, 0);

		/* put the list of arguments in char *proc_args */
		/* 
		 * XXX: if types were taken in consideration,
		 * a similar list for types would have to be
		 * built and codegen_proc() amended to accept
		 * it as an argument
		 */
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

		/* 
		 * Do the boilerplate routine stuff and code
		 * the procedure body (and symbols, etc.)
		 */
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

		/* 
		 * Move the arguments directly to the
		 * callee stack offsets
		 */
		for (i = tree->child[0]->child_count - 1; i >= 0; --i) {
			sto = codegen(tree->child[0]->child[i]);
			str = registerize(sto);
			/* XXX: assumes int arguments */
			printf("movl %s, %s\n", str, symstack((-2 - i) * 4));
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
		/* 
		 * Put the return expression's value
		 * in EAX and jump to the end of the
		 * routine
		 */
		printf("# return value\n");
		if (strcmp(sto, "%eax"))
			/* 
			 * XXX: this assumes an int return value
			 * (which is the default, anyway) 
			 */
			printf("movl %s, %%eax # ret\n", sto, str);
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
				sto = sym_lookup(tree->child[0]->tok);
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
		/* 
		 * XXX: assumes codegen() always
		 * returns int, which atm it does,
		 * but perhaps shouldn't
		 */
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
		/* 
		 * XXX: assumes codegen() always
		 * returns int, which atm it does,
		 * but perhaps shouldn't
		 */
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
		/* 
		 * XXX: assumes codegen() always
		 * returns int, which atm it does,
		 * but perhaps shouldn't
		 */
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

	/* 
	 * pre-increment, pre-decrement of variable lvalue
	 * -- this works for int, int *, char, char *
	 */
	if ((tree->head_type == INC
		|| tree->head_type == DEC)
		&& tree->child[0]->head_type == VARIABLE) {
		sym_s = sym_lookup(tree->child[0]->tok);
		typedat = sym_lookup_type(tree->child[0]->tok);
		if (typedat.ptr || typedat.arr) {
			/*
			 * It's a pointer, so use `l' suffix
			 * for the datum itself, but change
			 * by the size of the pointed-to
			 * elements.
			 */
			sprintf(sbuf, "%sl",
				tree->head_type == INC ? "add" : "sub");
			printf("%s $%d, %s\n",
				sbuf, type2siz(deref_typeof(typedat)), sym_s);
		} else {
			/*
			 * It's not a pointer, so use the
			 * type's own suffix and change by 1
			 */
			sprintf(sbuf, "%s%s",
				tree->head_type == INC ? "inc" : "dec",
				siz2suffix(type2siz(typedat)));
			printf("%s %s\n", sbuf, sym_s);
		}
		return sym_s;
	}

	/* pre-increment, pre-decrement of array lvalue */
	if ((tree->head_type == INC
		|| tree->head_type == DEC)
		&& tree->child[0]->head_type == ARRAY) {

		/* head address */
		sym_s = sym_lookup(tree->child[0]->child[0]->tok);

		/* index expression */
		str = codegen(tree->child[0]->child[1]);
		sto2 = registerize(str);

		/* build pointer */
		sto = get_temp_reg();
		/* XXX: FIXME: assumes int */
		printf("imull $4, %s\n", sto2);
		printf("addl %s, %s\n", sym_s, sto2);
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

	/* 
	 * post-increment, post-decrement of variable lvalue
	 * -- this works for int, int *, char, char *
	 */
	if ((tree->head_type == POST_INC
		|| tree->head_type == POST_DEC)
		&& tree->child[0]->head_type == VARIABLE) {
		/* given foo++,
		 * codegen foo, keep its value,
		 * codegen ++foo; 
		 * and give back the kept value */
		sto = codegen(tree->child[0]);
		fake_tree = new_exp_tree(tree->head_type == 
			POST_INC ? INC : DEC, NULL);
		add_child(&fake_tree, tree->child[0]);
		codegen(&fake_tree);
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

		/* XXX: assumes int */

		sto = registerize(codegen(tree->child[0]->child[0]));
		sto2 = registerize(codegen(tree->child[1]));
		printf("movl %s, (%s)\n", sto2, sto);

		return sto2;
	}

	/* simple variable assignment */
	if (tree->head_type == ASGN && tree->child_count == 2
		&& tree->child[0]->head_type == VARIABLE) {
		sym_s = sym_lookup(tree->child[0]->tok);
		/*
		 * XXX: handles char <- number constant,
		 *      int <- int, pointer <- pointer,
		 *      but not other cases like
		 *		char <- int, int <- char
		 */
		if (tree->child[1]->head_type == NUMBER) {
			/*
			 * optimized code for number operand 
			 */
			/* get type of dest */
			typedat = sym_lookup_type(tree->child[0]->tok);
			membsiz = type2siz(typedat);
			printf("mov%s", siz2suffix(membsiz));
			printf(" $%s, %s\n",
				get_tok_str(*(tree->child[1]->tok)), sym_s);
			return sym_s;
		} else if (tree->child[1]->head_type == VARIABLE) {
			/* optimized code for variable operand */
			sto = sym_lookup(tree->child[1]->tok);
			sto2 = get_temp_reg();
			printf("movl %s, %s\n", sto, sto2);
			printf("movl %s, %s\n", sto2, sym_s);
			free_temp_reg(sto2);
			return sym_s;
		} else {
			/* general case */
			sto = codegen(tree->child[1]);
			sto2 = registerize(sto);
			printf("movl %s, %s\n", sto2, sym_s);
			free_temp_reg(sto2);
			return sym_s;
		}
	}

	/* 
	 * array assignment
	 * -- seems to work for char, char *, int
	 */
	if (tree->head_type == ASGN && tree->child_count == 2
		&& tree->child[0]->head_type == ARRAY) {
		/* head address */
		sym_s = sym_lookup(tree->child[0]->child[0]->tok);
		membsiz = type2siz(
			deref_typeof(sym_lookup_type(tree->child[0]->child[0]->tok)));
		/* index expression */
		str = codegen(tree->child[0]->child[1]);
		sto2 = registerize(str);
		/* right operand */
		str2 = codegen(tree->child[1]);
		sto3 = registerize(str2);

		printf("imull $%d, %s\n", 
			membsiz, sto2);
		printf("addl %s, %s\n", sym_s, sto2);
		printf("mov%s %s, (%s)\n", 
			siz2suffix(membsiz),
			fixreg(sto3, membsiz),
			sto2);

		free_temp_reg(sto2);
		free_temp_reg(sto3);

		return sto3;
	}

	/* 
	 * array retrieval -- works for int, char, char *
	 * char data gets converted to an int register
	 */
	if (tree->head_type == ARRAY && tree->child_count == 2) {
		/* head address */
		sym_s = sym_lookup(tree->child[0]->tok);
		/* index expression */
		printf("# index expr\n");
		str = codegen(tree->child[1]);
		sto2 = registerize(str);
		/* member size */
		membsiz = type2siz(
			deref_typeof(sym_lookup_type(tree->child[0]->tok)));

		sto = get_temp_reg();
		printf("# build ptr\n");
		/* multiply offset by member size */
		printf("imull $%d, %s\n", membsiz, sto2);
		printf("addl %s, %s\n", sym_s, sto2);
		printf("%s (%s), %s\n", 
			move_conv_to_long(membsiz), sto2, sto);

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

	/* variable retrieval
	 * -- converts char to int
	 */
	if (tree->head_type == VARIABLE) {
		sto = get_temp_reg();
		sym_s = sym_lookup(tree->tok);
		membsiz = type2siz(sym_lookup_type(tree->tok));
		printf("%s %s, %s\n",
			move_conv_to_long(membsiz),
			sym_s, sto);
		return sto;
	}

	/* arithmetic */
	if ((arith = arith_op(tree->head_type)) && tree->child_count) {
		/* (with optimized code for number and variable operands
		 * that avoids wasting temporary registes) */
		sto = get_temp_reg();
		
		/*
		 * Check if "pointer arithmetic" is necessary --
		 * this happens in the case of additions containing
		 * at least one pointer-typed object.
		 */
		ptr_arith_mode = tree->head_type == ADD
						&& tree_typeof(tree).ptr;
		obj_siz = type2siz(tree_typeof(tree));
		ptr_count = 0;
		if (ptr_arith_mode) {
			for (i = 0; i < tree->child_count; ++i) {
				if (tree_typeof(tree->child[i]).ptr
					|| tree_typeof(tree->child[i]).arr) {
					ptr_memb = i;
					if (ptr_count++)
						codegen_fail("please don't add pointers",
							tree->child[i]->tok);
				}
			}
		}

		for (i = 0; i < tree->child_count; i++) {
			oper = i ? arith : "movl";
			/* 
			 * Pointer arithmetic mode -- e.g. if 
			 * `p' is an `int *', p + 5 becomes
			 * p + 5 * sizeof(int)
			 */
			if (ptr_arith_mode && i != ptr_memb) {
				str = codegen(tree->child[i]);
				printf("imull $%d, %s\n", obj_siz, str);
				printf("%s %s, %s\n", oper, str, sto);
				free_temp_reg(str);
			} else if (tree->child[i]->head_type == NUMBER) {
				printf("%s $%s, %s\n", 
					oper, get_tok_str(*(tree->child[i]->tok)), sto);
			} else if(tree->child[i]->head_type == VARIABLE) {
				sym_s = sym_lookup(tree->child[i]->tok);
				printf("%s %s, %s\n", oper, sym_s, sto);
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
	/* 
	 * XXX: assumes codegen() always
	 * returns int, which atm it does,
	 * but perhaps shouldn't
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
				sym_s = sym_lookup(tree->child[i]->tok);
				printf("idivl %s\n", sym_s);
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
	/* 
	 * XXX: assumes codegen() always
	 * returns int, which atm it does,
	 * but perhaps shouldn't
	 */
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
		/* 
		 * Check for an "else-return" pattern;
		 * if it is encountered, the "else"
		 * label and jump are not necessary 
		 */
		else_ret = tree->child_count == 3
		 && tree->child[2]->head_type == RET;
		/* 
		 * Jump over else block if there
		 * is one
		 */
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
	/* 
	 * XXX: assumes codegen() always
	 * returns int, which atm it does,
	 * but perhaps shouldn't
	 */
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
	/* 
	 * XXX: assumes codegen() always
	 * returns int, which atm it does,
	 * but perhaps shouldn't
	 */
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
/* 
 * XXX: assumes codegen() always
 * returns int, which atm it does,
 * but perhaps shouldn't
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
	/* 
	 * Check for else-return pattern;
	 * if it is encountered, the else
	 * label and jump are not necessary
	 */
	else_ret = tree->child_count == 3
	 && tree->child[2]->head_type == RET;
	/* 
	 * Jump over else block if there
	 * is one
	 */
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


