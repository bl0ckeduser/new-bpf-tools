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

#define SYMLEN 256

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
extern void compiler_debug(char *message, token_t *token,
	int in_line, int in_chr);
extern token_t *findtok(exp_tree_t *et);
extern int type2siz(typedesc_t);
extern typedesc_t mk_typedesc(int bt, int ptr, int arr);
extern int is_arith_op(char);
extern int type2offs(typedesc_t ty);
extern int struct_tag_offs(typedesc_t stru, char *tag_name);
extern typedesc_t struct_tree_2_typedesc(exp_tree_t *tree, int *bytecount,
	struct_desc_t **sd_arg);
extern int check_array(exp_tree_t *decl);
extern void parse_type(exp_tree_t *dc, typedesc_t *typedat,
				typedesc_t *array_base_type, int *objsiz, int decl);
extern void count_stars(exp_tree_t *dc, int *stars);
extern int get_arr_dim(exp_tree_t *decl, int n);
void setup_symbols(exp_tree_t* tree, int glob);
extern void discard_stars(exp_tree_t *dc);
char* get_tok_str(token_t t);
extern int arr_dim_prod(typedesc_t ty);
extern void parse_array_info(typedesc_t *typedat, exp_tree_t *et);

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
char current_proc[SYMLEN];

/* Table of local symbols */
char symtab[256][SYMLEN] = {""};
int syms = 0;			/* count */
int symbytes = 0;		/* stack size in bytes */
int symsiz[256] = {0};		/* size of each object */
typedesc_t symtyp[256];

/* Table of global symbols */
char globtab[256][SYMLEN] = {""};
int globs = 0;	/* count */
typedesc_t globtyp[256];

/* Table of (current) function-argument symbols */
char arg_symtab[256][SYMLEN] = {""};
int arg_syms = 0;		/* count */
int argbytes = 0;		/* total size in bytes */
int argsiz[256] = {0};		/* size of each object */
typedesc_t argtyp[256];

/* 
 * Table of argument and return type tables associated
 * to function names -- used for locally-declared
 * functions
 */
struct {
	typedesc_t argtyp[256];
	typedesc_t ret_typ;
	char name[SYMLEN];
	int argc;
} func_desc[256];
int funcdefs = 0;

/* 
 * Table used for tracking named structs
 * (e.g. struct bob { ... }; then struct bob bob_variable;)
 */
struct_desc_t* named_struct[256];
char named_struct_name[256][64];
int named_structs = 0;

/* Table of string-constant symbols */
char str_const_tab[256][SYMLEN] = {""};
int str_consts = 0;

/* Temporary-use registers currently in use */
char ts_used[TEMP_REGISTERS];

/* Temporary-use stack offsets currently in use */
char tm_used[TEMP_MEM];

int proc_ok = 1;
int else_ret;			/* used for "else return" optimization */
int main_defined = 0;		/* set if user-defined main() exists */
int ccid = 0;

int stack_size;
int intl_label = 0; 		/* internal label numbering */

char buf[1024];

/* 
 * Stuff for `break' labels in WHILEs.
 * note that the parser turns FORs into
 * equivalent WHILEs so effectively this only
 * applies to WHILEs
 */
int break_count = 0;
int break_labels[256];

/* ====================================================== */

struct_desc_t *find_named_struct_desc(char *s)
{
	int i;
	char err_buf[1024];
	for (i = 0; i < named_structs; ++i)
		if (!strcmp(named_struct_name[i], s))
			return named_struct[i];


	sprintf(err_buf, "unknown struct name `%s'", s);
	fail(err_buf);
}

typedesc_t func_ret_typ(char *func_nam)
{
	int i;
	for (i = 0; i < funcdefs; ++i)
		if (!strcmp(func_desc[i].name, func_nam))
			return func_desc[i].ret_typ;
	/*
	 * If the function is not found in the table,
	 * it's probably an external one like printf().
	 * Least insane behaviour is to default to int.
	 * XXX: search extern def table if any ever gets added
	 */
	return mk_typedesc(INT_DECL, 0, 0);
}

/*
 * Find number of arithmetic nodes
 * in a tree. (Used by the arithmetic
 * code writer to try to figure out
 * if it's going to run out of 
 * registers)
 */
int arith_depth(exp_tree_t *t)
{
	int d = 0, i;
	if (is_arith_op(t->head_type))
		++d;
	for (i = 0; i < t->child_count; ++i)
		d += arith_depth(t->child[i]);
	return d;
}

/* 
 * Checks for a plain "int" (without any pointer stars 
 * or array dimensions) 
 */
int is_plain_int(typedesc_t td)
{
	return td.ty == INT_DECL
		&& td.ptr == 0 
		&& td.arr == 0;
}

token_t argvartok(exp_tree_t *arg)
{
	if (arg->child[0]->head_type == VARIABLE)
		return *(arg->child[0]->tok);
	else
		return *(arg->child[1]->tok);
}

void codegen_fail(char *msg, token_t *tok)
{
	compiler_fail(msg, tok, 0, 0);
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

int free_register_count() {
	int i, c = 0;
	for (i = 0; i < TEMP_REGISTERS; ++i)
		if (!ts_used[i])
			++c;
	return c;
}

/*
 * A version of get_temp_reg() which tries
 * to deal with some x86 annoyances.
 */
char* get_temp_reg_siz(int siz) {
	int i;

	/* 
	 * For non-movb stuff, try to grab esi, edi
	 * first, because movb can't use it (see below)
	 */
	if (siz == 4) {
		for (i = 4 /* index of %esi */; i < TEMP_REGISTERS; ++i)
			if (!ts_used[i]) {
				ts_used[i] = 1;
				return temp_reg[i];
			}
	}

	for (i = 0; i < TEMP_REGISTERS; ++i)
		if (!ts_used[i]) {
			/* Error: `%esi' not allowed with `movb' */
			if (siz == 1 && !strcmp(temp_reg[i], "%esi"))
				continue;
			/* Error: `%edi' not allowed with `movb' */
			if (siz == 1 && !strcmp(temp_reg[i], "%edi"))
				continue;
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
		sprintf(buf, "%d(%%ebp)", -id);
	else
		sprintf(buf, "-%d(%%ebp)", id);
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
	if (strlen(s) >= SYMLEN)
		compiler_fail("symbol name too long", tok, 0, 0);
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
	if (strlen(s) >= SYMLEN)
		compiler_fail("symbol name too long", tok, 0, 0);
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
	if (strlen(s) >= SYMLEN)
		compiler_fail("string constant too long", tok, 0, 0);
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
 * Set up symbols and stack storage
 * for an (N-dimensional) array.
 * Returns the symbol number.
 */
int create_array(int symty, exp_tree_t *dc,
	int base_size, int objsiz)
{
	/* XXX: global arrays unsupported */
	if (symty == SYMTYPE_GLOBALS)
		codegen_fail("global arrays are currently unsupported",
			dc->child[0]->tok);

	/* Check that the array's variable name is not already taken */
	sym_check(dc->child[0]->tok);

	/* Make storage for all but the first entries 
	 * (the whole array is objsiz bytes, while one entry
	 * has size base_size)
	 */
	symsiz[syms++] = objsiz - base_size;
	symbytes += objsiz - base_size;

	/* Clear the symbol name tag for the symbol table
	 * space taken over by the array entries -- otherwise
	 * the symbol table lookup routines may give incorrect
	 * results if there is some leftover stuff from another
	 * procedure */
	*symtab[syms - 1] = 0;

	/* 
 	 * The symbol maps to the first element of the
	 * array, not a pointer to it. This is an important
	 * rule, according to "The Development of the C Language"
	 * (http://www.cs.bell-labs.com/who/dmr/chist.html).
	 * 
	 * The first element of the array is the last added here,
	 * because the x86 stack grows "backwards" and sym_add()
	 * follows the growth of the stack.
	 */
	return sym_add(dc->child[0]->tok, base_size);
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
		if (strlen(args[i]) >= SYMLEN)
			compiler_fail("argument name too long", findtok(tree), 0, 0);
		strcpy(arg_symtab[arg_syms], args[i]);
		++arg_syms;
	}

	/* Make the symbol and label for the procedure */
#ifdef MINGW_BUILD
	/* printf(".type _%s, @function\n", name); */
	printf("_%s:\n", name);
#else
	printf(".type %s, @function\n", name);
	printf("%s:\n", name);
#endif
	/* 
	 * Populate the symbol table with the
	 * function's local variables and set aside
	 * stack space for them
	 */
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
 * in a user's program. (If it isn't,
 * all the code at function level
 * is considered the main(), like in
 * losethos/templeOS's modified 
 * C / C++ compiler).
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
	 * the variables at function level
	 * are globals
	 */
	if (main_defined) {
		printf("# start globals =============\n");
		printf(".section .data\n");
		setup_symbols(tree, SYMTYPE_GLOBALS);
		printf(".section .text\n");
		printf("# end globals =============\n\n");
	}

	/* Don't ask me why this is necessary.
 	 * I probably pasted it from an assembly tutorial. */
	printf(".section .text\n");
#ifdef MINGW_BUILD
	printf(".globl _main\n\n");
#else
	printf(".globl main\n\n");
#endif

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
		codegen_proc("main", tree, main_args);

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
		for (i = 0; i < tree->child_count; ++i) {
			printout_tree(*(tree->child[i]));
			fprintf(stderr, "\n");
			if (tree->child[i]->head_type != NULL_TREE)
			 dump_td(tree_typeof(tree->child[i]));
		}
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
		 * to write them again. Hence their
		 * absence from this printf().
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
 * Byte size of an integer object
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

void setup_symbols(exp_tree_t *tree, int symty)
{
	extern void setup_symbols_iter(exp_tree_t *tree, int symty, int first_pass);
	setup_symbols_iter(tree, symty, 1);
	setup_symbols_iter(tree, symty, 0);
}

void setup_symbols_iter(exp_tree_t *tree, int symty, int first_pass)
{
	int i, j, k, sto;
	exp_tree_t *dc;
	int stars, newlen;
	char *str;
	int sym_num;
	int objsiz;
	int start_bytes;
	int decl = tree->head_type;
	typedesc_t typedat, struct_base, tag_type;
	int array_bytes;
	int dim;
	typedesc_t array_base_type;
	struct_desc_t* sd;
	int tag_offs;
	char tag_name[64];
	int struct_bytes;
	typedesc_t *heap_typ;
	int padding;
	int struct_pass;
	exp_tree_t inits;
	exp_tree_t init_expr;
	exp_tree_t *initializer_val;
	int children_offs;

	/*
	 * Handle structs in the second pass
	 */
	if ((tree->head_type == STRUCT_DECL
		|| tree->head_type == NAMED_STRUCT_DECL) && !first_pass) {
		if (tree->head_type == STRUCT_DECL) {
			/* read the struct declaration parse tree */
			struct_base = struct_tree_2_typedesc(tree, &struct_bytes, &sd);
			struct_base.struct_desc->bytes = struct_bytes;

			/* register the struct's name (e.g. "bob" in 
			 * "struct bob { ... };") */
			named_struct[named_structs] = struct_base.struct_desc;
			strcpy(named_struct_name[named_structs],
				get_tok_str(*(tree->tok)));
			++named_structs;
			children_offs = sd->cc;
		} else {
			/* NAMED_STRUCT_DECL -- use existing named struct type */
			struct_base.ty = 0;		
			struct_base.arr = struct_base.ptr = 0;
			struct_base.is_struct = 1;
			struct_base.is_struct_name_ref = 0;
			sd = struct_base.struct_desc = 
				find_named_struct_desc(get_tok_str(*(tree->tok)));
			children_offs = 0;
			struct_bytes = struct_base.struct_desc->bytes;
		}

		/* XXX: global structs unsupported */
		if (symty == SYMTYPE_GLOBALS && tree->child_count - children_offs > 0)
			codegen_fail("global structs are unsupported",
				findtok(tree));

		/* discard the tree from further codegeneneration */
		tree->head_type = NULL_TREE;

		/* create initializers tree */
		inits = new_exp_tree(BLOCK, NULL);

		/* 
		 * Now do the declaration children (i.e. the actual
		 * variables, with pointer/array modifiers and 
		 * symbol name identifiers, after the "struct { ... } " part)
		 */
		for (i = children_offs; i < tree->child_count; ++i) {
			dc = tree->child[i];

			/* 
			 * The type of the declaration child is derived
			 * from the struct type by adding pointer or array
			 * qualifications
			 */

			typedat = struct_base;
			parse_array_info(&typedat, dc);
			count_stars(dc, &(typedat.ptr));

			#ifdef DEBUG
				fprintf(stderr, "struct `%s' -> ",
					get_tok_str(*(tree->tok)));
				fprintf(stderr, "decl var `%s`, qual a=%d,p=%d \n",
					get_tok_str(*(dc->child[0]->tok)),
					typedat.arr, typedat.ptr);
			#endif


			/*
			 * If there is an initializer, add it to the initializer
			 * code tree
			 */
			if (dc->child_count - typedat.arr - typedat.ptr - 1) {
				for (j = 1; j < dc->child_count; ++j)
					if (!(dc->child[j]->head_type == ARRAY_DIM
						|| dc->child[j]->head_type == DECL_STAR)) {
						initializer_val = dc->child[j];
						break;
					}
				init_expr = new_exp_tree(ASGN, NULL);
				add_child(&init_expr, dc->child[0]);
				add_child(&init_expr, initializer_val);
				add_child(&inits, alloc_exptree(init_expr));

				#ifdef DEBUG
					fprintf(stderr, "initializer >>>>>>>>\n");
					printout_tree(*dc);
					fprintf(stderr, "\n");
					printout_tree(init_expr);
					fprintf(stderr, "\n");
					fprintf(stderr, ">>>>>>>>>>>>>>>>>>>>\n");
				#endif
			}

			/* 
			 * XXX: if global structs / struct arrays / etc. 
			 * were supported dispatches to the global symbol table
			 * would be needed around here
			 */

			/* 
			 * Figure out the size of the object
			 * (if it's an array, this is the size of the base object)
			 */
			if (typedat.ptr)
				/* pointers are always 4 bytes on 32-bit x86
				 * hurr durr amirite */
				objsiz = 4;
			else
				/* not a pointer: it's just the size of the struct */
				objsiz = struct_bytes;

			if (typedat.arr) {
				/* 
				 * N-dimensional array of struct (or of N-degree 
				 * pointers to struct)
				 */
				array_base_type = typedat;
				array_base_type.arr = 0;
				sym_num = create_array(symty, dc,
					type2siz(array_base_type), arr_dim_prod(typedat) * objsiz);
				symtyp[sym_num] = typedat;
			} else {
				/*
				 * It's just a non-arrayed variable (either a struct
				 * or an N-degree pointer thereof); just add it to
				 * the symbol table.
				 */
				sym_num = sym_add(dc->child[0]->tok, objsiz);
				symtyp[sym_num] = typedat;
			}
		}

		/* 
		 * Copy the initializers code tree to the original
		 * tree pointer, so it'll get codegenerated during
		 * the main codegeneration pass which deals with
		 * code rather than declarations 
 		 */
		memcpy(tree, &inits, sizeof (exp_tree_t));

		return;
	}

	/*
	 * Handle integer (char, int) types and pointers/arrays of them
	 */
	if (int_type_decl(decl)) {
		for (i = 0; i < tree->child_count; ++i) {
			dc = tree->child[i];

			/* 
			 * Do all the smaller stuff first;
			 * for some reason, if I put ints at
			 * big offsets like -2408(%ebp),
			 * segfaults happen. (???)
			 * 
			 * Maybe it's actually an alignment
			 * issue, but googling gives nothing
			 * and I'm not smart and brave enough
			 * to spend 3 hours reading Intel PDFs.
			 * And anyway this is just a toy project,
			 * innit ?
			 */
			if (first_pass && check_array(dc))
				continue;
			if (!first_pass && !check_array(dc))
				continue;

			/* 
			 * Get some type information about the object
			 * declared
			 */
			parse_type(dc, &typedat, &array_base_type,
						&objsiz, decl);
			discard_stars(dc);

			/* 
			 * Switch symbols/stack setup code
			 * based on array dimensions of object
			 */
			if (check_array(dc) > 0) {
				/* Create the memory and symbols */
				sym_num = create_array(symty, dc,
					type2siz(array_base_type), objsiz);

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
							sym_num = sym_add(dc->child[0]->tok, objsiz);
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
				 * For stack variables, turn the initial value node
				 * into an assignment in the body of the
				 * procedure; otherwise, discard the tree 
				 */
				if (dc->child_count == 2 
					&& symty == SYMTYPE_STACK)
					(*dc).head_type = ASGN;
				else
					(*dc).head_type = NULL_TREE;
			}

			/*
			 * Store type information about the object
			 * in the appropriate symbol table (local or global)
		 	 */
			if (symty == SYMTYPE_STACK) {				
				symtyp[sym_num] = typedat;
			} else if (symty == SYMTYPE_GLOBALS) {
				globtyp[sym_num] = typedat;
			}
		}
		
		/*
		 * Turn the declaration sequence into
		 * a standard code sequence, because at
		 * this point the assignments are ASGN
		 * and the rest are NULL_TREE and the
		 * stack variable initializers (the ASGNs)
		 * have to be coded.
		 */
		if (!first_pass)
			tree->head_type = BLOCK;
	}

	/* Child recursion */
	for (i = 0; i < tree->child_count; ++i)
		if (tree->child[i]->head_type == BLOCK
		||	tree->child[i]->head_type == IF
		||	tree->child[i]->head_type == WHILE
		||	int_type_decl(tree->child[i]->head_type)
		||  (!first_pass && tree->child[i]->head_type == STRUCT_DECL)
		||  (!first_pass && tree->child[i]->head_type == NAMED_STRUCT_DECL))
			setup_symbols_iter(tree->child[i], symty, first_pass);
}

/*
 * If a temporary storage location is already
 * a register, give it back as is. Otherwise,
 * copy it to a new temporary register and give
 * this new register. (This is used because
 * several x86 instructions seem to expect
 * registers rather than stack offsets).
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
 * Convert integer data of size `fromsiz' to int and move 
 * it into a new int register
 */
char *registerize_from(char *stor, int fromsiz)
{
	int is_reg = 0;
	int i;
	char* str;
	
	str = get_temp_reg_siz(fromsiz);
	printf("%s %s, %s\n", 
		move_conv_to_long(fromsiz), 
		fixreg(stor, fromsiz), str, fromsiz);
	return str;
}

char* registerize_siz(char *stor, int siz)
{
	int is_reg = 0;
	int i;
	char* str;

	/* Error: `%esi' not allowed with `movb' */
	/* Error: `%edi' not allowed with `movb' */
	if (siz == 1 
		&& (!strcmp(stor, "%esi") || !strcmp(stor, "%edi"))) {
		free_temp_reg(stor);
	}
	else
		for (i = 0; i < TEMP_REGISTERS; ++i)
			if (!strcmp(stor, temp_reg[i]))
				return stor;
	
	str = get_temp_reg_siz(siz);
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
	int membsiz, offs_siz;
	typedesc_t typedat;
	typedesc_t *callee_argtyp = NULL;
	int callee_argbytes;	
	int offset;
	int stackstor;
	exp_tree_t *argl, *codeblock;
	int custom_return_type;
	int ptr_diff;
	int do_deref;

/*
	if (findtok(tree))
		compiler_debug("trying to compile this line",
			findtok(tree), 0, 0);		
*/

	if (tree->head_type == BLOCK
		|| tree->head_type == IF
		|| tree->head_type == WHILE
		|| tree->head_type == BPF_INSTR) {
		/* clear temporary memory and registers */
		new_temp_mem();
		new_temp_reg();
	}

	/* 
	 * -> read
	 */
	if (tree->head_type == DEREF_STRUCT_MEMB) {
		char buf[128];
		strcpy(buf, get_tok_str(*(tree->child[1]->tok)));
		int offs = struct_tag_offs(tree_typeof(tree->child[0]),
						buf);

		printf("# -> load tag `%s' with offset %d\n", buf, offs);

		membsiz = type2siz(tree_typeof(tree));

		/* get base adr */
		if (tree->child[0]->head_type != VARIABLE)
			sym_s = registerize(codegen(tree->child[0]));
		else
			sym_s = sym_lookup(tree->child[0]->tok);

		sto2 = get_temp_reg();

		/* build pointer */
		printf("movl %s, %s\n", sym_s, sto2);
		free_temp_reg(sym_s);
		printf("addl $%d, %s\n", offs, sto2);
	
		/* 
		 * Yet another subtlety...
		 * if the tag is an array,
		 * do not dereference, because
		 * arrays evaluate to pointers at run-time.
		 */
		if (tree_typeof(tree).arr) {
			/* return pointer */ 
			return sto2;
		} else {
			/* dereference */ 
			sto = get_temp_reg();
			printf("%s (%s), %s\n", 
				move_conv_to_long(membsiz), sto2, sto);
			free_temp_reg(sto2);
			return sto;
		}
	}

	/*
	 * -> write
	 */
	if (tree->head_type == ASGN
		&& tree->child[0]->head_type == DEREF_STRUCT_MEMB) {
		char buf[128];
		strcpy(buf, get_tok_str(*(tree->child[0]->child[1]->tok)));
		int offs = struct_tag_offs(tree_typeof(tree->child[0]->child[0]),
						buf);

		printf("# load address of tag `%s' with offset %d\n", buf, offs);

		membsiz = type2siz(tree_typeof(tree));

		/* get base adr */
		if (tree->child[0]->child[0]->head_type != VARIABLE)
			sym_s = registerize(codegen(tree->child[0]->child[0]));
		else
			sym_s = sym_lookup(tree->child[0]->child[0]->tok);

		sto2 = get_temp_reg();

		/* build pointer */
		printf("movl %s, %s\n", sym_s, sto2);
		free_temp_reg(sym_s);
		printf("addl $%d, %s\n", offs, sto2);
	
		sto3 = registerize(codegen(tree->child[1]));
		printf("movl %s, (%s)\n", sto3, sto2);
		free_temp_reg(sto2);
		return sto3;
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

	/* 
	 * When arrays are evaluated, the result is a
	 * pointer to their first element (see earlier
	 * comments about the paper called 
	 * "The Development of the C Language"
	 * and what it says about this)
	 */
	if (tree->head_type == VARIABLE
		&& sym_lookup_type(tree->tok).arr) {
		/* build pointer to first member of array */
		sto = get_temp_reg_siz(4);
		printf("leal %s, %s\n",
			sym_lookup(tree->tok),
			sto);
		return sto;
	}

	/* "bob123" */
	if (tree->head_type == STR_CONST) {
		sto = get_temp_reg_siz(4);
		printf("movl $_str_const_%d, %s\n",
			str_const_lookup(tree->tok),
			sto);
		return sto;
	}

	/* &x */
	if (tree->head_type == ADDR
		&& tree->child_count == 1
		&& tree->child[0]->head_type == VARIABLE) {
		
		sto = get_temp_reg_siz(4);

		/* LEA: load effective address */
		printf("leal %s, %s\n",
				sym_lookup(tree->child[0]->tok),
				sto);

		return sto;
	}

	/* &(a[v]) = a + membsiz * v 
	 * -- this works for `a' of type int, char, char **,
	 * (and probably anything else at this point),
	 * at least assuming the index expression cleanly
	 * compiles to an int. note that it is legal for
	 * all the instructions to have `l' suffix because
	 * we're dealing with pointers.
	 */
	if (tree->head_type == ADDR
		&& tree->child_count == 1
		&& tree->child[0]->head_type == ARRAY) {

		offs_siz = type2offs(deref_typeof(
			tree_typeof(tree->child[0]->child[0])));

		membsiz = type2siz(deref_typeof(
			tree_typeof(tree->child[0]->child[0])));

		/* get base pointer */
		sto = codegen(tree->child[0]->child[0]);

		/* 
		 * now we code the array index expression.
		 * codegen() should convert
		 * whatever it encounters to `int',
		 * so no type-conversion worries
		 * if the array index is not an `int'
		 */
		sto2 = registerize_siz(codegen(tree->child[0]->child[1]),
				membsiz);

		/* multiply index byte-offset by size of members */
		if (offs_siz != 1)
			printf("imull $%d, %s\n", offs_siz, sto2);

		/* ptr = base_adr + membsiz * index_expr */
		printf("addl %s, %s\n", sto2, sto);
		free_temp_reg(sto2);

		return sto;
	}

	/* *(exp) */
	if (tree->head_type == DEREF
		&& tree->child_count == 1) {
		/* 
		 * Find byte size of *exp
	     */
		membsiz = type2siz(
			deref_typeof(tree_typeof(tree->child[0])));
		
		sto = registerize_siz(codegen(tree->child[0]), membsiz);
		sto2 = get_temp_reg_siz(membsiz);


		/* Check for crazy wack situations like *3 */
		if (membsiz <= 0) {
			membsiz = 1;
			compiler_warn("you're dereferencing what doesn't"
						  " seem to be a pointer.\nsegfaults might"
						  " be coming your way soon.",
						  findtok(tree), 0, 0);
		}

		/* deref and convert to int -- because
		 * the rest of the code assumes codegen()
		 * gives ints */
		printf("%s (%s), %s\n", 
			move_conv_to_long(membsiz),
			sto, sto2);
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
		 * Get argument size data, if the procedure
		 * has been declared.
		 */
		/* XXX: extern declarations would go in another table */
		callee_argtyp = NULL;
		for (i = 0; i < funcdefs; ++i) {
			if (!strcmp(func_desc[i].name,
				get_tok_str(*(tree->tok)))) {
					callee_argtyp = func_desc[i].argtyp;
					/*
					 * Check for arg count mismatch
					 */
					if (tree->child_count != func_desc[i].argc) {
						sprintf(sbuf, "Argument count mismatch -- "
									  "`%s' expects %d arguments but "
									  "you have given it %d",
										func_desc[i].name,
										func_desc[i].argc,
										tree->child_count);
						compiler_fail(sbuf,
							findtok(tree), 0, 0);
					}
					break;
				}
		}

		/* 
		 * Compute stack size of all the arguments together
		 */
		if (!callee_argtyp) {
			/* default is int */
			compiler_warn("no declaration found, "
				      "assuming all arguments are 32-bit int",
				findtok(tree), 0, 0);
			/* sizeof(int) = 4 on 32-bit x86 */
			callee_argbytes = tree->child_count * 4;
		}
		else {
			/* sum individual offsets */
			callee_argbytes = 0;
			for (i = 0; i < tree->child_count; ++i)
				callee_argbytes += type2siz(callee_argtyp[i]);
		}

		/* 
		 * Move down the stack pointer
		 */
		printf("subl $%d, %%esp\n", callee_argbytes);

		/* 
		 * Move the arguments into the new stack offsets
		 */
		offset = 0;
		for (i = 0; i < tree->child_count; ++i) {
			/* Find out the type of the current argument */
			if (!callee_argtyp) {
				/* Default to int if no declaration 
				 * provided */
				typedat.ty = INT_DECL;
				typedat.ptr = typedat.arr = 0;

				/* offset = sizeof(int) * i */
				offset = 4 * i;
			} else {
				/* load registered declaration */
				typedat = callee_argtyp[i];
			}

			/* Get byte size of current argument */
			membsiz = type2siz(typedat);

			/* XXX: might have to convert args to 
			 * function's arg type */
			sto = registerize(codegen(tree->child[i]));
			/* XXX: use movb et al ? */
			printf("movl %s, %d(%%esp)\t",
				sto, offset);
			printf("# argument %d to %s\n",
				i, get_tok_str(*(tree->tok)));
			free_temp_reg(sto);

			if (callee_argtyp)
				offset += type2siz(callee_argtyp[i]);
		}

		/* 
		 * Call the subroutine
		 */
		#ifdef MINGW_BUILD
			printf("call _%s\n", get_tok_str(*(tree->tok)));
		#else
			printf("call %s\n", get_tok_str(*(tree->tok)));
		#endif

		/* 
		 * Throw off the arguments from the stack
		 */
		printf("addl $%d, %%esp\t# throw off arg%s\n",
			callee_argbytes,
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
		/*
		 * Can't nest procedure definitions, not in C, anyway
		 */
		if (!proc_ok)
			compiler_fail("function definition not allowed here (it's nested)",
				findtok(tree), 0, 0);

		/* 
		 * Put the list of arguments in char *proc_args,
		 * and also populate the argument type descriptions table
		 */
		custom_return_type = 0;
		if (tree->child[0]->head_type == CAST_TYPE) {
			custom_return_type = 1;
			argl = tree->child[1];
			compiler_warn("only int- or char- sized return types work",
							findtok(tree),
							0, 0);
		}
		else
			argl = tree->child[0];

		argbytes = 0;
		if (argl->child_count) {
			for (i = 0; argl->child[i]; ++i) {
				/* 
				 * Copy this argument's variable name to proc_args[i]
				 */
				buf = malloc(64);
				strcpy(buf, get_tok_str
					(argvartok(argl->child[i])));
				proc_args[i] = buf;

				#ifdef DEBUG
					/* debug-print argument type info */
					fprintf(stderr, "%s: \n",
						get_tok_str(argvartok(argl->child[i])));
					dump_td(tree_typeof(argl->child[i]));
				#endif

				/* Obtain and store argument type data */
				argtyp[i] = tree_typeof(argl->child[i]);

				/* If no type specified, default to int */
				if (argtyp[i].ty == TO_UNK)
					argtyp[i].ty = INT_DECL;

				/* Calculate byte offsets */
				argsiz[i] = type2siz(argtyp[i]);
				argbytes += argsiz[i];

			}
			proc_args[i] = NULL;
		} else
			*proc_args = NULL;

		/* 
		 * Register function typing info
		 */
		/* argument types */
		for (i = 0; i < argl->child_count; ++i)
			func_desc[funcdefs].argtyp[i] = argtyp[i];
		/* name */
		strcpy(func_desc[funcdefs].name, get_tok_str(*(tree->tok)));
		/* arg count */
		func_desc[funcdefs].argc = argl->child_count;
		/* return type */
		if (custom_return_type)
			func_desc[funcdefs].ret_typ = tree_typeof(tree);
		else
			/* default return type is "int" */
			func_desc[funcdefs].ret_typ = mk_typedesc(INT_DECL, 0, 0);
		funcdefs++;

		/* prevent nested procedure defs */
		proc_ok = 0;

		/* 
		 * Do the boilerplate routine stuff and code
		 * the procedure body (and symbols, etc.)
		 */
		if (tree->child[0]->head_type == CAST_TYPE)
			codeblock = tree->child[2];
		else
			codeblock = tree->child[1];
		buf = malloc(64);
		/* procedure name */
		strcpy(buf, get_tok_str(*(tree->tok)));
		codegen_proc(buf,
			codeblock,
			proc_args);
		free(buf);

		/* okay you can define procedures again now */
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
			/* clear temporary memory and registers */
			new_temp_mem();
			new_temp_reg();
			codegen(tree->child[i]);
		}
		return NULL;
	}

	/* ! -- logical not (0 if X is not 1) */
	if (tree->head_type == CC_NOT) {
		my_ccid = ccid++;
		sto = codegen(tree->child[0]);
		sto2 = get_temp_reg_siz(4);
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
		sto2 = get_temp_reg_siz(4);
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
		sto2 = get_temp_reg_siz(4);
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

	/* pre-increment, pre-decrement of array lvalue
	 * -- has been tested for array of char, int, char*  */
	if ((tree->head_type == INC
		|| tree->head_type == DEC)
		&& tree->child[0]->head_type == ARRAY) {

		/* get base pointer */
		sym_s = codegen(tree->child[0]->child[0]);

		/* index expression */
		str = codegen(tree->child[0]->child[1]);
		sto2 = registerize(str);

		/* byte size of array members */
		membsiz = type2siz(
			deref_typeof(sym_lookup_type(tree->child[0]->child[0]->tok)));
		offs_siz = type2offs(
			deref_typeof(sym_lookup_type(tree->child[0]->child[0]->tok)));

		/* build pointer */
		sto = get_temp_reg_siz(4);
		if (offs_siz != 1)
			printf("imull $%d, %s\n", offs_siz, sto2);
		printf("addl %s, %s\n", sym_s, sto2);
		printf("movl %s, %s\n", sto2, sto);
		free_temp_reg(sto2);
		free_temp_reg(sym_s);

		/* write the final move */
		sto3 = get_temp_reg_siz(membsiz);
		printf("%s%s (%s)\n", 
			tree->head_type == DEC ? "dec" : "inc",
			siz2suffix(membsiz),
			sto);

		/* convert final result to int */
		compiler_debug("array lval preinc -- trying conversion",
			findtok(tree), 0, 0);
		printf("%s (%s), %s\n", 
			move_conv_to_long(membsiz),
			sto, sto3);
		free_temp_reg(sto);
		return sto3;
	}

	/* 
	 * pre-increment, pre-decrement of pointer
	 * dereference.
	 *
	 * tested with ++*ptr for ptr of type char *
	 */
	if ((tree->head_type == INC
		|| tree->head_type == DEC)
		&& tree->child[0]->head_type == DEREF) {
		/* byte size of pointee (sic) */
		membsiz = type2siz(deref_typeof(
			tree_typeof(tree->child[0]->child[0])));

		sto = registerize_siz(codegen(tree->child[0]->child[0]),
				membsiz);

		printf("%s", tree->head_type == INC ?
					 "inc" : "dec");

		printf("%s (%s)\n", 
			siz2suffix(membsiz),
			sto);

		return sto;
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
		/* byte size of pointee (sic) */
		membsiz = type2siz(deref_typeof(
			tree_typeof(tree->child[0]->child[0])));

		sto = registerize_siz(codegen(tree->child[0]->child[0]),
				membsiz);
		sto2 = registerize_siz(codegen(tree->child[1]),
				membsiz);

		printf("mov%s %s, (%s) #ptra\n", 
			siz2suffix(membsiz),
			fixreg(sto2, membsiz),
			sto);

		return sto2;
	}

	/* simple variable assignment */
	/* XXX: TODO: struct assignments */
	if (tree->head_type == ASGN && tree->child_count == 2
		&& tree->child[0]->head_type == VARIABLE) {
		sym_s = sym_lookup(tree->child[0]->tok);
		/* 
		 * XXX: might not handle some complicated cases
		 * properly yet
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
		} else if (tree->child[1]->head_type == VARIABLE
			&& type2siz(tree_typeof(tree->child[1])) 
				== type2siz(tree_typeof(tree->child[0]))) {
			/* optimized code for two operands of same size */
			membsiz = type2siz(tree_typeof(tree->child[0]));
			sto = sym_lookup(tree->child[1]->tok);
			sto2 = get_temp_reg_siz(membsiz);
			printf("mov%s %s, %s\n", 
				siz2suffix(membsiz),
				fixreg(sto, membsiz), 
				fixreg(sto2, membsiz));
			printf("mov%s %s, %s\n", 
				siz2suffix(membsiz),
				fixreg(sto2, membsiz), 
				sym_s);
			free_temp_reg(sto2);
			return sym_s;
		} else if (type2siz(tree_typeof(tree->child[0])) == 4) {
			/* general case for 4-byte destination */
			membsiz = type2siz(tree_typeof(tree->child[1]));
			/* n.b. codegen() converts stuff to int */
			sto = registerize_siz(codegen(tree->child[1]), membsiz);
			compiler_debug("simple variable assignment -- "
						  " looking for conversion suffix",
						  findtok(tree), 0, 0);
			sto2 = registerize_from(sto, membsiz);
			printf("movl %s, %s\n", sto2, sym_s);
			free_temp_reg(sto2);
			return sym_s;
		} else if (type2siz(tree_typeof(tree->child[0])) == 1) {
			/* general case for 1-byte destination */
			sto = codegen(tree->child[1]);	/* converts stuff to int */
			sto2 = registerize_siz(sto, 1);
			printf("movb %s, %s\n", fixreg(sto2, 1), sym_s);
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
		/* get base ptr */
		sym_s = codegen(tree->child[0]->child[0]);
		/* member size */
		membsiz = type2siz(
			deref_typeof(tree_typeof(tree->child[0]->child[0])));
		offs_siz = type2offs(
			deref_typeof(tree_typeof(tree->child[0]->child[0])));
		/* index expression */
		str = codegen(tree->child[0]->child[1]);
		sto2 = registerize_siz(str, membsiz);

		if (offs_siz != 1)
			printf("imull $%d, %s\n", offs_siz, sto2);
		printf("addl %s, %s\n", sym_s, sto2);
		free_temp_reg(sym_s);

		/* right operand */
		str2 = codegen(tree->child[1]);
		sto3 = registerize_siz(str2, membsiz);

		printf("mov%s %s, (%s) #arra\n", 
			siz2suffix(membsiz),
			fixreg(sto3, membsiz),
			sto2);

		free_temp_reg(sto2);
		free_temp_reg(sto3);

		return sto3;
	}

	/* 
	 * Array retrieval -- works for int, char, char *
	 * char data gets converted to an int register
	 */
	if (tree->head_type == ARRAY
		 && tree->child_count == 2) {
		/* get base ptr */
		sym_s = codegen(tree->child[0]);

		/* index expression */
		str = codegen(tree->child[1]);
		sto2 = registerize(str);
		/* member size */
		membsiz = type2siz(
			deref_typeof(tree_typeof(tree->child[0])));
		offs_siz = type2offs(
			deref_typeof(tree_typeof(tree->child[0])));

		sto = get_temp_reg_siz(membsiz);
		/* multiply offset by member size */
		if (offs_siz != 1)
			printf("imull $%d, %s\n", offs_siz, sto2);
		printf("addl %s, %s\n", sym_s, sto2);
		free_temp_reg(sym_s);
		compiler_debug("array retrieval -- trying conversion",
			findtok(tree), 0, 0);

		/*
		 * Assuming the declaration "int mat[20][30]",
		 * 
		 * mat[i][j] = *(&mat[0] + i * 30 * sizeof(int)
		 *	             + j * sizeof(int))
		 * 
		 * which means that the inner expression mat[i] 
		 * does *NOT* trigger a dereference upon evaluation.
		 * If it weren't for this subtlety, instead the compiler
		 * would compile it as
		 * 
		 *  *(*(&mat[0] + i * 30 * sizeof(int))
		 *  	 + j * sizeof(int))
		 * 
		 * which assumes that mat[i] is a legal pointer, which it
		 * isn't because array symbols are not compiled as pointers,
		 * but only converted to pointers at evaluation.
		 * 
		 * Now, what about e.g. int *mat[20] ? Then mat[i] is a legal
		 * pointer and in that case, you do dereference.
		 */
		if (tree_typeof(tree).arr)
			do_deref = 0;
		else
			do_deref = 1;

		if (do_deref)
			printf("%s (%s), %s\n", 
				move_conv_to_long(membsiz), sto2, sto);
		else
			printf("movl %s, %s\n", 
				sto2, sto);		

		free_temp_reg(sto2);

		return sto;
	}

	/* 
	 * general-case pre-increment, pre-decrement,
	 * (used for e.g. ++a->b)
	 * (some specific cases like ++a get handled before this)
	 */
	if (tree->head_type == INC || tree->head_type == DEC) {
		fake_tree = new_exp_tree(ASGN, NULL);
		fake_tree_2 = new_exp_tree(tree->head_type == INC ? ADD : SUB, NULL);
		add_child(&fake_tree_2, tree->child[0]);
		add_child(&fake_tree_2, alloc_exptree(one_tree));
		add_child(&fake_tree, tree->child[0]);
		add_child(&fake_tree, alloc_exptree(fake_tree_2));
		return codegen(alloc_exptree(fake_tree));
	}

	/* 
	 * general-case post-increment, post-decrement,
	 * (used for e.g. a.b++)
	 * (some specific cases like a++ get handled before this)
	 */
	if (tree->head_type == POST_INC || tree->head_type == POST_DEC) {
		/* cache old value */
		sto = registerize(codegen(tree->child[0]));
		/* code the bump by emitting a "X = X + 1" tree */
		fake_tree = new_exp_tree(ASGN, NULL);
		fake_tree_2 = new_exp_tree(tree->head_type == POST_INC ? ADD : SUB, NULL);
		add_child(&fake_tree_2, tree->child[0]);
		add_child(&fake_tree_2, alloc_exptree(one_tree));
		add_child(&fake_tree, tree->child[0]);
		add_child(&fake_tree, alloc_exptree(fake_tree_2));
		free_temp_reg(codegen(alloc_exptree(fake_tree)));
		/* give back the cached value */
		return sto;
	}

	/* number */
	if (tree->head_type == NUMBER) {
		sto = get_temp_reg_siz(4);
		printf("movl $%s, %s\n", 
			get_tok_str(*(tree->tok)),
			sto);
		return sto;
	}

	/* variable retrieval
	 * -- converts char to int
	 */
	if (tree->head_type == VARIABLE) {
		sto = get_temp_reg_siz(4);
		sym_s = sym_lookup(tree->tok);
		membsiz = type2siz(sym_lookup_type(tree->tok));
		compiler_debug("variable retrieval -- trying conversion",
			findtok(tree), 0, 0);
		printf("%s %s, %s\n",
			move_conv_to_long(membsiz),
			sym_s, sto);
		return sto;
	}

	/* arithmetic */
	if ((arith = arith_op(tree->head_type)) && tree->child_count) {
		/* 
		 * Set aside the temporary storage for the result
		 * as a register if there's enough left to go through
		 * with the calculation using exclusively 
		 * registers. Otherwise, use stack.
		 */
		if (free_register_count() > 3 * arith_depth(tree)) {
			sto = get_temp_reg();
			stackstor = 0;
		}
		else {
			sto = get_temp_mem();
			stackstor = 1;
		}

		/* 
		 * Check if "pointer arithmetic" is necessary --
		 * this happens in the case of additions or subtractions
		 * containing at least one pointer-typed object.
		 * See K&R 2nd edition (ANSI C89), page 205-206, A7.7
		 */
		ptr_arith_mode = (tree->head_type == ADD
						|| tree->head_type == SUB)
						&& tree_typeof(tree).ptr;

		/* If the pointer type is char *, then the mulitplier,
		 * obj_siz = sizeof(char) -- make sure to deref
		 * the type before asking for its size ! */
		if (ptr_arith_mode) 
			obj_siz = type2offs(deref_typeof(tree_typeof(tree)));

		/* Find out which of the operands is the pointer,
		 * so that it won't get multiplied. if there is
		 * more than one pointer operand, complain and fail. */
		ptr_count = 0;
		if (ptr_arith_mode) {
			for (i = 0; i < tree->child_count; ++i) {
				if (tree_typeof(tree->child[i]).ptr
					|| tree_typeof(tree->child[i]).arr) {
					ptr_memb = i;
					if (ptr_count++)
						codegen_fail("please don't + or - several "
									 " pointers",
							tree->child[i]->tok);
				}
			}
		}

		for (i = 0; i < tree->child_count; i++) {
			oper = i ? arith : "movl";
			/* 
			 * Pointer arithmetic mode -- e.g. if 
			 * `p' is an `int *', p + 5 becomes
			 * p + 5 * sizeof(int) --
			 * 
			 * See K&R 2nd edition (ANSI C89) page 205, A7.7
			 */
			if (ptr_arith_mode && i != ptr_memb) {
				str = registerize(codegen(tree->child[i]));
				if (obj_siz != 1)
					printf("imull $%d, %s\n", obj_siz, str);
				printf("%s %s, %s\n", oper, str, sto);
				free_temp_reg(str);
			} else if (tree->child[i]->head_type == NUMBER) {
				str = registerize(sto);
				printf("%s $%s, %s\n", 
					oper, get_tok_str(*(tree->child[i]->tok)), str);
				if (stackstor) {
					printf("movl %s, %s\n", str, sto);			
					free_temp_reg(str);
				}
			} else {
				/* general case. note that codegen() always gives an int */
				str = registerize(codegen(tree->child[i]));
				str2 = registerize(sto);
				printf("%s %s, %s\n", oper, str, str2);
				if (stackstor) {
					printf("movl %s, %s\n", str2, sto);
					free_temp_reg(str2);
				}
				free_temp_reg(str);
			}
		}
		if (stackstor)
			free_temp_mem(sto);
		return registerize(sto);
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
		/* move result (in EAX or EDX depending on operation)
		 * to some temporary storage */
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
		str2 = get_temp_reg_siz(4);
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
		str2 = get_temp_reg_siz(4);
		printf("movl $0, %s\n", str2);
		printf("cmpl %s, %s\n", str, str2);
		free_temp_reg(sto);
		free_temp_reg(str);
		free_temp_reg(str2);
		printf("je IL%d\n",	lab2);
		/* open break-scope */
		break_labels[++break_count] = lab2;
		/* codegen the block */
		codegen(tree->child[1]);
		/* jump back to the conditional */
		printf("jmp IL%d\n", lab1);
		printf("IL%d: \n", lab2);
		/* close break-scope */
		--break_count;
		return NULL;
	}

	/* break */
	if (tree->head_type == BREAK) {
		if (!break_count)
			codegen_fail("break outside of a loop", tree->tok);
		printf("jmp IL%d # break\n", break_labels[break_count]);
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
		sto2 = get_temp_reg_siz(4);
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


	/*
	 * Failed to codegen
	 */
	if (findtok(tree)) {
		compiler_warn("I don't yet know how to code this", findtok(tree), 0, 0);
		fprintf(stderr, "\n\n");
	}
	fprintf(stderr, "codegen_x86: incapable of coding tree:\n");
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
	sto3 = get_temp_reg_siz(4);	/* result */
	printf("movl $0, %s\n", sto3);
	str = registerize(codegen(tree->child[0]));
	str2 = registerize(codegen(tree->child[1]));
	printf("cmpl %s, %s\n", str2, str);
	free_temp_reg(str);
	free_temp_reg(str2);
	printf("%s IL%d\n", oppcheck, intl_label);
	printf("movl $1, %s\n", sto3);
	printf("IL%d: \n", intl_label++);
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
	/* open break-scope */
	break_labels[++break_count] = lab2;
	/* codegen the block */
	codegen(tree->child[1]);
	/* jump back to the conditional */
	printf("jmp IL%d\n", lab1);
	printf("IL%d: \n", lab2);
	/* close break-scope */
	--break_count;
	return NULL;
}


