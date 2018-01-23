/* 
 * Jan 2018
 * this is based on copying codegen_x86.c and tweaking it somewhat.
 * it still has bugs and missing features, possibly both subtle!!!
 * also the way it deals with the weird amd64 calling convention
 * is a bit messy, frankly x86 seemed easier to work with
 * anyway the cool thing is now you can just have 64-bit ints easily
 * and do 64-bit calculations, see test/amd64/slow/euler77.c for a "fun"
 * example
 *
 * ------------------------------------------------------------------
 *
 * Syntax tree ==> 64-bit x86 assembly (GAS syntax) generator 
 * 
 * It's my first time (quickly) doing x86 so it's probably far
 * from optimal. The essence of this module is juggling with
 * registers and coping with permitted parameter-types. 
 * Many improvements are possible.
 *
 * Note that this code generator uses the standard C boolean
 * idiom of 0 = false, nonzero = true, unlike the BPFVM one.
 */

/*
 * The entry point is "run_codegen"
 */

/* ====================================================== */

#define SYMLEN 256
#define INIT_BUF_SIZ 32

#include "constfold.h"
#include "hashtable.h"
#include "tree.h"
#include "tokens.h"
#include "typedesc.h"
#include "codegen_x86.h"
#include "general.h"
#include <string.h>
#include <stdio.h>
#include "diagnostics.h"

extern int get_global_initializer_string_gnux86(exp_tree_t *t, char *str_dest);

extern int type2siz(typedesc_t);
extern typedesc_t mk_typedesc(int bt, int ptr, int arr);
extern int is_arith_op(char);
extern int type2offs(typedesc_t ty);
extern int struct_tag_offs(typedesc_t stru, char *tag_name);
extern typedesc_t struct_tree_2_typedesc(exp_tree_t *tree, int *bytecount,
	struct_desc_t **sd_arg);
extern int check_array(exp_tree_t *decl);
extern void parse_type(exp_tree_t *dc, typedesc_t *typedat,
	typedesc_t *array_base_type, int *objsiz, int decl, exp_tree_t * father);
extern void count_stars(exp_tree_t *dc, int *stars);
extern int get_arr_dim(exp_tree_t *decl, int n);
void setup_symbols(exp_tree_t* tree, int glob);
extern void discard_stars(exp_tree_t *dc);
char* get_tok_str(token_t t);
extern int arr_dim_prod(typedesc_t ty);
extern void parse_array_info(typedesc_t *typedat, exp_tree_t *et);
void create_jump_tables(exp_tree_t*);

/* ====================================================== */


#define TEMP_REGISTERS 6
char* temp_reg[TEMP_REGISTERS];

#define AMD64_NUMBER_OF_CALLING_REGISTERS 6
char *amd64_calling_registers[AMD64_NUMBER_OF_CALLING_REGISTERS];

#define TEMP_MEM 16
char* temp_mem[TEMP_MEM];

char symstack_buf[128];
char tokstr_buf[1024];

int old_size;

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

/*
 * XXX: all these stupid O(n) arrays
 * could and should be converted to use
 * the new hashtable.
 * UPDATE: partly the code relies on arrays,
 * so specifically the lookup part
 * would be the thing to modify to use hashtables
 */

/* Name of function currently being coded */
char current_proc[SYMLEN];

/* Table of (current) local symbols */
/* (only one procedure is ever being codegen'd at a given time) */ 
char **symtab;
int symtab_a = INIT_BUF_SIZ;

int syms = 0;			/* count */
int symbytes = 0;		/* stack size in bytes */
int *symsiz;			/* size of each object */
int symsiz_a = INIT_BUF_SIZ;

typedesc_t *symtyp;		/* type description of each object */
int symtyp_a = INIT_BUF_SIZ;

/* Table of global symbols */
char **globtab;
int globtab_a = INIT_BUF_SIZ;
int globs = 0;	/* count */
typedesc_t *globtyp;
int globtyp_a = INIT_BUF_SIZ;

/* Table of (current) function-argument symbols */
/* (only one procedure is ever being codegen'd at a given time) */
char **arg_symtab;
int arg_symtab_a = INIT_BUF_SIZ;
int arg_syms = 0;		/* count */
int argbytes = 0;		/* total size in bytes */
int *argsiz;			/* size of each object */
int argsiz_a = INIT_BUF_SIZ;
typedesc_t* argtyp;		/* type description of each object */
int argtyp_a = INIT_BUF_SIZ;

/* same thing but for prototypes */
char **proto_arg_symtab;
int proto_arg_symtab_a = INIT_BUF_SIZ;
int proto_arg_syms = 0;		/* count */
int proto_argbytes = 0;		/* total size in bytes */
int *proto_argsiz;			/* size of each object */
int proto_argsiz_a = INIT_BUF_SIZ;
typedesc_t* proto_argtyp;		/* type description of each object */
int proto_argtyp_a = INIT_BUF_SIZ;

/*
 * Table of argument and return type tables associated
 * to function names -- used for locally-declared
 * functions
 */
typedef struct {
	typedesc_t argtyp[256];
	typedesc_t ret_typ;
	char name[SYMLEN];
	int argc;
} func_desc_t;

func_desc_t *func_desc;
int func_desc_a = INIT_BUF_SIZ;
int funcdefs = 0;

/*
 * Table used for tracking named structs
 * (as in e.g. "struct bob { ... };"
 * followed by "struct bob bob_variable;")
 */
struct_desc_t** named_struct;
int named_struct_a = INIT_BUF_SIZ;
char **named_struct_name;
int named_struct_name_a = INIT_BUF_SIZ;
int named_structs = 0;

/* Table of string-constant symbols */
hashtab_t* str_consts;
int str_const_id = 0;

/* Temporary-use registers currently in use */
char ts_used[TEMP_REGISTERS];

/* Temporary-use stack offsets currently in use */
char tm_used[TEMP_MEM];

int proc_ok = 1;		/* this is used to prevent nested procedure defs
			   	 * it is raised whenever a procedure is not being coded */

int ccid = 0;			/* internal label numbering,
				 * used specifically for short-circuiting
				 * booleans like || and && */

int intl_label = 0; 		/* internal label numbering */

char buf[1024];			/* general use */

int switch_count = 0;		/* index of a switch statement */
				/* (counts; used for building jump tables) */
/*
 * Stuff for `break' (and `continue')
 * labels in WHILEs. Note that the parser
 * turns FORs into equivalent WHILEs so effectively
 * this only applies to WHILE nodes
 */
int break_count = 0;
int break_labels[256];

/* 
 * Tree currently being coded.
 * used for error-message purposes.
 */
exp_tree_t *codegen_current_tree = NULL;

int switch_maxlab[256];

/* ====================================================== */

/*
 * Helper: mark all registers needed to pass the arguments
 * as dirty (AMD64 special)
 */
void amd64_dirty_call_registers(exp_tree_t *tree)
{
	int i, j;
	for (i = 0; i < tree->child_count; ++i) {
		for (j = 0; j < TEMP_REGISTERS; ++j) {
			if (!strcmp(amd64_calling_registers[i], temp_reg[j])) {
				ts_used[j] = 1;
			}
		}
	}
}

void* checked_malloc(long s)
{
	void *ptr = malloc(s);
	if (!ptr)
		fail("malloc failed");
	return ptr;
}

/*
 * expand buffers if necessary
 */
void expandBuffers()
{
	int i;

	if (syms >= symtab_a || syms >= symsiz_a || syms >= symtyp_a) {
		old_size = symtab_a;
		while (syms >= symtab_a || syms >= symsiz_a || syms >= symtyp_a) {
			symtab_a *= 2;
			symsiz_a *= 2;
			symtyp_a *= 2;
		}
		symtab = realloc(symtab, symtab_a * sizeof(char *));
		for (i = old_size; i < symtab_a; ++i) {
			symtab[i] = checked_malloc(256);
			*symtab[i] = '\0';
		}
		symsiz = realloc(symsiz, symsiz_a * sizeof(int));
		symtyp = realloc(symtyp, symtyp_a * sizeof(typedesc_t));
	}

	if (globs >= globtab_a || globs >= globtyp_a) {
		old_size = globtab_a;
		while (globs >= globtab_a || globs >= globtyp_a) {
			globtab_a *= 2;
			globtyp_a *= 2;
		}
		globtab = realloc(globtab, globtab_a * sizeof(char *));
		for (i = old_size; i < globtab_a; ++i) {
			globtab[i] = checked_malloc(256);
			*globtab[i] = '\0';
		}
		globtyp = realloc(globtyp, globtyp_a * sizeof(typedesc_t));
	}

	if (arg_syms >= arg_symtab_a || arg_syms >= argsiz_a || arg_syms >= argtyp_a) {
		old_size = arg_symtab_a;
		while (arg_syms >= arg_symtab_a || arg_syms >= argsiz_a || arg_syms >= argtyp_a) {
			arg_symtab_a *= 2;
			argsiz_a *= 2;
			argtyp_a *= 2;
		}
		arg_symtab = realloc(arg_symtab, arg_symtab_a * sizeof(char *));
		for (i = old_size; i < arg_symtab_a; ++i) {
			arg_symtab[i] = checked_malloc(256);
			*arg_symtab[i] = '\0';
		}
		argsiz = realloc(argsiz, argsiz_a * sizeof(int));
		argtyp = realloc(argtyp, argtyp_a * sizeof(typedesc_t));
	}

	if (named_structs >= named_struct_a || named_structs >= named_struct_name_a) {
		old_size = named_struct_name_a;
		while (named_structs >= named_struct_a || named_structs >= named_struct_name_a) {
			named_struct_a *= 2;
			named_struct_name_a *= 2;
		}
		named_struct = realloc(named_struct, named_struct_a * sizeof(struct_desc_t *));
		named_struct_name = realloc(named_struct_name, named_struct_name_a * sizeof(char *));
		for (i = old_size; i < named_struct_name_a; ++i) {
			named_struct_name[i] = checked_malloc(64);
			*named_struct_name[i] = '\0';
		}
	}

	if (proto_arg_syms >= proto_arg_symtab_a || proto_arg_syms >= proto_argsiz_a || proto_arg_syms >= proto_argtyp_a) {
		old_size = proto_arg_symtab_a;
		while (proto_arg_syms >= proto_arg_symtab_a || proto_arg_syms >= proto_argsiz_a || proto_arg_syms >= proto_argtyp_a) {
			proto_arg_symtab_a *= 2;
			proto_argsiz_a *= 2;
			proto_argtyp_a *= 2;
		}
		proto_arg_symtab = realloc(proto_arg_symtab, proto_arg_symtab_a * sizeof(char *));
		for (i = old_size; i < proto_arg_symtab_a; ++i) {
			proto_arg_symtab[i] = checked_malloc(256);
			*proto_arg_symtab[i] = '\0';
		}
		proto_argsiz = realloc(proto_argsiz, proto_argsiz_a * sizeof(int));
		proto_argtyp = realloc(proto_argtyp, proto_argtyp_a * sizeof(typedesc_t));
	}

}

void register_structs(exp_tree_t *tree)
{
	typedesc_t struct_base;
	int struct_bytes;
	int i;
	struct_desc_t* sd;

	for (i = 0; i < tree->child_count; ++i)
		if (tree->child[i]->head_type == BLOCK ||
		    tree->child[i]->head_type == STRUCT_DECL) {
		register_structs(tree->child[i]);
	}

	if (tree->head_type == STRUCT_DECL) {
		#ifdef DEBUG
			fprintf(stderr, "struct variable with base struct %s...\n", 
				get_tok_str(*(tree->tok)));
		#endif
		/* parse the struct declaration syntax tree */
		struct_base = struct_tree_2_typedesc(tree, &struct_bytes, &sd);
		struct_base.struct_desc->bytes = struct_bytes;

		/* 
		 * Register the struct's name (e.g. "bob" in 
		 * "struct bob { ... };"), because it might
		 * be referred to simply by its name later on
		 * (in fact that's what the else-clause below 
		 * deals with)
		 * 
		 * XXX: wait, what if it's anonymous ?
		 */
		named_struct[named_structs] = struct_base.struct_desc;
		strcpy(named_struct_name[named_structs],
			get_tok_str(*(tree->tok)));
		++named_structs;
	}
}

/*
 * Check for an expression that is either:
 * - a procedure call
 * - an assignment of a procedure call to a variable
 *
 * This is needed because the implementation is designed
 * in such a way procedure calls need special
 * treatment when used as expressions.
 */
int check_proc_call(exp_tree_t *et)
{
	int i;
	if (!et)
		return 0;
	if (et->head_type == PROC_CALL)
		return 1;
	if (et->head_type == ASGN && et->child[0]->head_type == VARIABLE)
		for (i = 0; i < et->child_count; ++i)
			if (check_proc_call(et->child[i]))
				return 1;
	return 0;
}

/*
 * Look up the struct description table
 * for a struct name tag, e.g. "bob"
 * from "struct bob { int x; int y}; "
 */
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

/*
 * Look up the return type information
 * for a function, starting from its name
 */
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
 * Checks if the provided type description
 * describes a plain "int" 
 * (without any pointer stars or array dimensions) 
 */
int is_plain_int(typedesc_t td)
{
	return (td.ty == INT_DECL || td.ty == LONG_DECL)
		&& td.ptr == 0 
		&& td.arr == 0;
}

/*
 * Get the variable name token object
 * from a procedure argument node
 */
token_t argvartok(exp_tree_t *arg)
{
	if (arg->child[0]->head_type == VARIABLE)
		return *(arg->child[0]->tok);
	else
		return *(arg->child[1]->tok);
}

/*
 * Emit a verbose error diagnostic message 
 * that puts a little arrow under the error, etc.
 * Like in "enterprise-quality" compilers.
 */
void codegen_fail(char *msg, token_t *tok)
{
	compiler_fail(msg, tok, 0, 0);
}

/*
 * This compiler uses GNU i386 assembler (or compatible), invoked
 * via "gcc" or "clang" for assembling.
 *
 * GNU i386 assembler version 2.22 gives messages like this:
 * "Warning: using `%bl' instead of `%rbx' due to `b' suffix"
 * when it sees code like "movb %rbx, (%rax)", but it compiles
 * without any further issues.
 *
 * GNU i386 assembler version 2.17.50 gives the diagnostic
 * "error: invalid operand for instruction" for the same code and
 * fails to compile.
 *
 * So here is a quick fix.
 * 
 * I'm certainly not an x86 expert (or an anything expert), 
 * so some of this might come off as noob-tier ad-hoc shit, indeed
 */
char *fixreg(char *r, int siz)
{
	char *new;
	if (siz == 1) {
		if (strlen(r) == 4 && r[0] == '%'
			&& r[1] == 'r' && r[3] == 'x') {
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
			return "movsbq";
		case 4:
			return "movl";
		case 8:
			return "movq";
		default:
			compiler_fail("data size mismatch -- something"
				      " doesn't fit into something else",
				findtok(codegen_current_tree), 0, 0);
	}
}

/* 
 * This is required by main(), because
 * it is used in the BPFVM codegen
 * which was written before this one.
 * The BPF vm codegen does some backpatching
 * so it cannot linearly barf out code as it
 * goes. This codegen needs no backpatching,
 * because the problem of forward label branches 
 * is left to the assembler to deal with it.
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
 * Find out how many registers are free
 */
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
	if (siz == 8) {
		for (i = 4 /* index of %rsi */; i < TEMP_REGISTERS; ++i)
			if (!ts_used[i]) {
				ts_used[i] = 1;
				return temp_reg[i];
			}
	}

	for (i = 0; i < TEMP_REGISTERS; ++i)
		if (!ts_used[i]) {
			/* Error: `%rsi' not allowed with `movb' */
			if (siz == 1 && !strcmp(temp_reg[i], "%rsi"))
				continue;
			/* Error: `%rdi' not allowed with `movb' */
			if (siz == 1 && !strcmp(temp_reg[i], "%rdi"))
				continue;
			ts_used[i] = 1;
			return temp_reg[i];
		}
	fail("out of registers");
}


/* 
 * Explicitly and voluntarily 
 * let go of a temporary register 
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
 * Explicitly and voluntarily let go
 * of a temporary stack memory index
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
	if (!id)
		sprintf(symstack_buf, "(%%rbp)");
	else if (id < 0)
		sprintf(symstack_buf, "%d(%%rbp)", -id);
	else
		sprintf(symstack_buf, "-%d(%%rbp)", id);
	return symstack_buf;
}

/* 
 * Extract the raw string 
 * from a token object 
 */
char* get_tok_str(token_t t)
{
	strncpy(tokstr_buf, t.start, t.len);
	tokstr_buf[t.len] = 0;
	return tokstr_buf;
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
		if (!strcmp(globtab[i], s) && globtyp[i].is_extern == 0) {
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

	expandBuffers();

	return symstack((symbytes += siz) - siz);
}

/*
 * Create a new symbol, obtaining its permanent
 * storage address.
 */ 
int sym_add(token_t *tok, int size)
{
	char *s = get_tok_str(*tok);
	int i;

	/*
	 * when it's a large struct,
	 * add all but last 8 bytes
	 * first, then after that add
	 * the last 8 bytes. all this
	 * because the stack grows
	 * downwards
	 */
	if (size > 8) {

		/* Check that the variable name is not already taken */
		sym_check(tok);

		/* 
		 * Make storage for all but the first entries 
		 * (the whole array is objsiz bytes, while one entry
		 * has size base_size)
		 */
		symsiz[syms++] = size - 8;
		symbytes += size - 8;

		expandBuffers();

		/* 
		 * Clear the symbol name tag for the symbol table
		 * space taken over by the array entries -- otherwise
		 * the symbol table lookup routines may give incorrect
		 * results if there is some leftover stuff from another
		 * procedure
		 */
		*symtab[syms - 1] = 0;

	}

	if (strlen(s) >= SYMLEN)
		compiler_fail("symbol name too long", tok, 0, 0);
	strcpy(symtab[syms], s);
	symsiz[syms] = 8;
	symbytes += 8;

	++syms;

	expandBuffers();

	return syms - 1; 
}


/*
 * Create a new global symbol
 */ 
int glob_add(token_t *tok)
{
	char *s = get_tok_str(*tok);
	if (strlen(s) >= SYMLEN)
		compiler_fail("symbol name too long", tok, 0, 0);
	strcpy(globtab[globs], s);
	++globs;

	expandBuffers();

	return globs - 1; 
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
 * Find the address of a symbol on the stack
 */
int sym_stack_offs(token_t *tok)
{
	char buf[1024];
	char *s = get_tok_str(*tok);
	int i = 0;

	/* Try stack locals */
	for (i = 0; i < syms; i++)
		if (!strcmp(symtab[i], s)) {
			return offs(symsiz, i);
		}

	/* Try arguments */
	/*
	 * On AMD64 arguments are usually not passed on the stack
	 * so this is commented-out.
	 */
	/*
	for (i = 0; i < arg_syms; i++)
		if (!strcmp(arg_symtab[i], s))
			return -2*8 - offs(argsiz, i);
	*/

	compiler_fail("could not find stack address for this symbol", tok, 0, 0);
	return 0;
}

/* 
 * Find the GAS syntax for the address of 
 * a symbol, which could be a local stack
 * symbol, a global, or a function argument
 * (which is actually also a stack offset,
 * like stack locals, but in a funny
 * position).
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
			return newstr(symstack(-2*8 - offs(argsiz, i)));

	/* 
	 * Try globals last -- they are shadowed
	 * by locals and arguments 
	 */
	for (i = 0; i < globs; ++i)
		if (!strcmp(globtab[i], s))
			return globtab[i];

	/*
	 * If it isn't anywhere, just assume
	 * it's an external symbol. The mini
	 * compiler otcc (http://bellard.org/otcc/)
	 * seems to behave this way in order
	 * to allow the use of "stderr" 
	 * without a working preprocessor.
	 * Perhaps it would be nice to pop up a
	 * warning.
	 * 
	 * The bad part of this is that undeclared
	 * variables only get spotted once you get
	 * to the assembler.
	 */
	compiler_warn("assuming symbol is external", tok, 0, 0);
	return s;

/*
	sprintf(buf, "unknown symbol `%s'", s);
	compiler_fail(buf, tok, 0, 0);
*/
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

	/* 
	 * Assume it's an external int
	 * (in parallel with similar behaviour
	 * in sym_lookup)
	 */
	return mk_typedesc(INT_DECL, 0, 0);

/*
	sprintf(buf, "unknown symbol `%s'", s);
	compiler_fail(buf, tok, 0, 0);
*/
}

/* 
 * Lookup storage number of a string constant
 * (string constants are all put in a bunch
 * of global symbols of the form "_str_const_XXX")
 */
int str_const_lookup(token_t* tok)
{
	int *val;

	val = hashtab_lookup(str_consts, get_tok_str(*tok));	

	if (!val) {
		sprintf(buf, "unregistered string constant `%s'", get_tok_str(*tok));
		compiler_fail(buf, tok, 0, 0);
	}

	return *val;
}

/* 
 * Add a string constant to the string constant table
 */
int str_const_add(token_t *tok)
{
	int *iptr = malloc(sizeof(int));
	if (!iptr)
		fail("malloc an int");
	hashtab_insert(str_consts, get_tok_str(*tok), iptr);
	return *iptr = str_const_id++;
}

/* 
 * Arithmetic expression tree type -> x86 instruction
 * (this is for the easy general
 * cases -- division and remainders
 * need special attention on X86,
 * since they aren't one-operation
 * things, are finnicky about which
 * registers you use, etc.)
 */
char* arith_op(int ty)
{
	switch (ty) {
		case ADD:
			return "addq";
		case SUB:
			return "subq";
		case MULT:
			return "imulq";
		/* 
		 * Note: bitwise ops &, ^, | are implemented
		 * using the same code as general arithmetic
		 * ops because it's simpler that way
		 */
		case BOR:
			return "orq";
		case BAND:
			return "andq";
		case BXOR:
			return "xorq";
		default:
			return 0;
	}
}

/*
 * Find out if a tree has a node of
 * type `nodety' anywhere in its whole
 * body
 */
int checknode(exp_tree_t *t, int nodety)
{
	int i;
	if (t->head_type == nodety)
		return 1;
	for (i = 0; i < t->child_count; ++i)
		if (checknode(t->child[i], nodety))
			return 1;
	return 0;
}

/*
 * Set up symbols and stack storage
 * for an (N-dimensional) array.
 * Returns the symbol number.
 */
int create_array(int symty, exp_tree_t *dc,
	int base_size, int objsiz, int is_extern)
{
	/* 
	 * First, try global symbol mode.
	 * Otherwise, it's a stack local.
	 */

	if (symty == SYMTYPE_GLOBALS) {
		if (!is_extern) {
			/* Check name not already taken */	
			glob_check(dc->child[0]->tok);

			/* register as global (needed for multifile) */
			printf(".globl %s\n", 
				get_tok_str(*(dc->child[0]->tok)));

			/* Do a magic assembler instruction */
			printf(".comm %s,%d,32\n", 
				get_tok_str(*(dc->child[0]->tok)),
				objsiz);
		}

		/* 
		 * XXX: can't do e.g. "int foo[] = {1, 2, 3}" declarations
		 * for globals
		 */
		if (checknode(dc, COMPLICATED_INITIALIZER))
			compiler_fail("global complicated"
				      " array-initializations unsupported",
				      findtok(dc), 0, 0);

		/* 
		 * Add symbol to the global symbol table
		 * and return table index
		 */
		return glob_add(dc->child[0]->tok);
	}

	/* Check that the array's variable name is not already taken */
	sym_check(dc->child[0]->tok);

	/* 
	 * Make storage for all but the first entries 
	 * (the whole array is objsiz bytes, while one entry
	 * has size base_size)
	 */
	symsiz[syms++] = objsiz - base_size;
	symbytes += objsiz - base_size;

	expandBuffers();

	/* 
	 * Clear the symbol name tag for the symbol table
	 * space taken over by the array entries -- otherwise
	 * the symbol table lookup routines may give incorrect
	 * results if there is some leftover stuff from another
	 * procedure
	 */
	*symtab[syms - 1] = 0;

	/* 
 	 * The symbol maps to the first element of the
	 * array, not a pointer to it. This is an important
	 * rule, according to "The Development of the C Language"
	 * (http://www.cs.bell-labs.com/who/dmr/chist.html).
	 * (It's not immediately obvious just by looking at C
	 * that things are implemented this way)
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
 * and also its argument tree expressions list
 * write the code for a procedure, and set up
 * the memory, stack, symbols, etc. needed.
 */
void codegen_proc(char *name, exp_tree_t *tree, char **args, exp_tree_t *argl)
{
	char buf[1024];
	char *buf2;
	int i;

	/* If syms is set to 0, funny things happen */
	syms = 1;
	symbytes = 8;
	symsiz[0] = 8;
	arg_syms = 0;

	strcpy(current_proc, name);


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
	 * stack space for them. This is kind of
	 * a messy process.
	 */
	setup_symbols(tree, SYMTYPE_STACK);

	/*
	 * Set up some general purpose temporary
	 * stack memory
	 * (there aren't so many registers, you know,
	 * so sometimes you have to ``spill to stack''
	 * as they say)
	 */
	for (i = 0; i < TEMP_MEM; ++i) {
		buf2 = malloc(64);
		strcpy(buf2, nameless_perm_storage(8));
		temp_mem[i] = buf2;
	}

	/*
	 * export symbol
	 */
#ifdef MINGW_BUILD
	printf(".globl _%s\n", name);
#else
	printf(".globl %s\n", name);
#endif

	/* 
	 * Do the usual x86 function entry process,
	 * moving down the stack pointer to make
	 * space for the local variables 
	 */

	printf("# set up stack space\n");
	printf("pushq %%rbp\n");
	printf("movq %%rsp, %%rbp\n");
	printf("subq $%d, %%rsp\n", symbytes);
	printf("\n# >>> compiled code for '%s'\n", name);

	printf("_tco_%s:\n", name);	/* hook for TCO */

	/*
	 * Copy the argument symbols to the *main* symbol table
	 * and copy them from the registers into local stack
	 * (special hack for AMD64) 
	 */
	for (i = 0; i < argl->child_count && args[i]; ++i) {
		if (strlen(args[i]) >= SYMLEN)
			compiler_fail("argument name too long", findtok(tree), 0, 0);
		/* strcpy(arg_symtab[arg_syms], args[i]); */
		/* ++arg_syms; */

		printf("movq %s, %s\n", amd64_calling_registers[i], symstack(symbytes));

		strcpy(symtab[syms], args[i]);

		/* Obtain and store argument type data */
		symtyp[syms] = tree_typeof(argl->child[i]);

		/*
			fprintf(stderr, "-----------> argument: %s\n", args[i]);
			printout_tree(*(argl->child[i]));
			dump_td(symtyp[syms]);
		*/

		/* If no type specified, default to int */
		if (symtyp[syms].ty == TO_UNK)
			symtyp[syms].ty = INT_DECL;

		/* Calculate byte offsets */
		symsiz[syms] = type2siz(symtyp[syms]);
		symbytes += symsiz[syms];

		++syms;

		expandBuffers();
	}

	/* 
	 * Code the body of the procedure
	 */
	codegen(tree);

	/* Do a typical x86 function return */
	printf("\n# clean up stack and return\n");
	printf("_ret_%s:\n", name);	/* hook for 'return' */
	printf("addq $%d, %%rsp\n", symbytes);
	printf("movq %%rbp, %%rsp\n");
	printf("popq %%rbp\n");
	printf("ret\n\n");
}

/* 
 * Entry point of the x86 codegen
 */
void run_codegen(exp_tree_t *tree)
{
	char *main_args[] = { NULL };
	extern void deal_with_procs(exp_tree_t *tree);
	extern void deal_with_str_consts(exp_tree_t *tree);
	int i;

	char* temp_reg_tmp[TEMP_REGISTERS] = {
		"%rax",
		"%rbx",
		"%rcx",
		"%rdx",
		"%rsi",
		"%rdi"
	};
	for (i = 0; i < sizeof(temp_reg_tmp)/sizeof(char*); ++i)
		temp_reg[i] = temp_reg_tmp[i];

	/*
	 * See https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI
	 */
	char* amd64_calling_registers_temp[AMD64_NUMBER_OF_CALLING_REGISTERS] = {
		"%rdi",
		"%rsi",
		"%rdx",
		"%rcx",
		"%r8",
		"%r9" 
	};
	for (i = 0; i < sizeof(amd64_calling_registers_temp)/sizeof(char*); ++i)
		amd64_calling_registers[i] = amd64_calling_registers_temp[i];

	/*
	 * re-initialize global counters
	 */
	syms = 0;			/* count */
	symbytes = 0;			/* stack size in bytes */
	globs = 0;			/* count */
	arg_syms = 0;			/* count */
	argbytes = 0;			/* total size in bytes */
	proto_arg_syms = 0;		/* count */
	proto_argbytes = 0;		/* total size in bytes */
	funcdefs = 0;
	named_structs = 0;
	str_const_id = 0;
	ccid = 0;			/* internal label numbering, */
	intl_label = 0; 		/* internal label numbering */
	switch_count = 0;		/* index of a switch statement */
	break_count = 0;


	/*
	 * set up hash table for string constants
	 */
	str_consts = new_hashtab();

	/*
	 * set up dynamic arrays
	 */
	symtab = checked_malloc(symtab_a * sizeof(char *));
	for (i = 0; i < symtab_a; ++i) {
		symtab[i] = checked_malloc(256);
		*symtab[i] = 0;
	}

	symsiz = checked_malloc(symsiz_a * sizeof(int));
	for (i = 0; i < symsiz_a; ++i)
		symsiz[i] = 0;

	symtyp = checked_malloc(symtyp_a * sizeof(typedesc_t));

	globtab = checked_malloc(globtab_a * sizeof(char *));
	for (i = 0; i < globtab_a; ++i) {
		globtab[i] = checked_malloc(256);
		*globtab[i] = 0;
	}

	globtyp = checked_malloc(globtyp_a * sizeof(typedesc_t));

	arg_symtab = checked_malloc(arg_symtab_a * sizeof(char *));
	for (i = 0; i < arg_symtab_a; ++i) {
		arg_symtab[i] = checked_malloc(256);
		*arg_symtab[i] = 0;
	}

	argsiz = checked_malloc(argsiz_a * sizeof(int));
	for (i = 0; i < argsiz_a; ++i)
		argsiz[i] = 0;
	
	argtyp = checked_malloc(argtyp_a * sizeof(typedesc_t));

	proto_arg_symtab = checked_malloc(proto_arg_symtab_a * sizeof(char *));
	for (i = 0; i < proto_arg_symtab_a; ++i) {
		proto_arg_symtab[i] = checked_malloc(256);
		*proto_arg_symtab[i] = 0;
	}

	proto_argsiz = checked_malloc(proto_argsiz_a * sizeof(int));
	for (i = 0; i < proto_argsiz_a; ++i)
		proto_argsiz[i] = 0;
	
	proto_argtyp = checked_malloc(proto_argtyp_a * sizeof(typedesc_t));

	func_desc = checked_malloc(func_desc_a * sizeof(func_desc_t));
	
	named_struct = checked_malloc(named_struct_a * sizeof(struct_desc_t*));

	named_struct_name = checked_malloc(named_struct_name_a * sizeof(char *));
	for (i = 0; i < named_struct_name_a; ++i) {
		named_struct_name[i] = checked_malloc(256);
		*named_struct_name[i] = 0;
	}

	/*
	 * Code the user's string constants.
	 */
	printf(".section .rodata\n");
	printf("# start string constants ========\n");
	deal_with_str_consts(tree);
	printf("# end string constants ==========\n\n");

	/*
	 * Create the jump tables for switch statements
	 * (AFAIK they should be going in rodata here).
	 * This will only create a bunch of pointers, the
	 * switch statements themselves are coded in the main
	 * codegen loop. This routine will "decorate" the SWITCH
	 * tree nodes with information linking them back to their
	 * appropriate pointers.
	 */
	create_jump_tables(tree);
	printf("\n");

	/*
	 * Do struct declarations in global lexical scope
	 */
	register_structs(tree);

	/*
	 * Do globals
	 */
	printf("# start globals =============\n");
	printf(".section .data\n");
	/*
	 * some space for big struct-typed return values
	 * 1024 bytes maximum
	 */
	printf(".comm __return_buffer,1024,32\n");
	setup_symbols(tree, SYMTYPE_GLOBALS);
	printf(".section .text\n");

	/*
	 * On FreeBSD 9.0 i386, the symbol for stdin 
	 * is __stdinp (on linux it's just stdin)
	 * (It's even funkier on MinGW, so much so
	 * that this kind of symbol hack isn't enough
	 * and several instructions are necessary,
	 * which is why the MinGW version of this hack
	 * is in the main codegen routine and not here).
	 */
	#ifdef __FreeBSD__
		printf("\n");
		printf("# ------ macro hack to get stdin/stdout symbols on freebsd --- # \n");
		printf(".set stdin, __stdinp\n");
		printf(".set stdout,  __stdoutp\n");
		printf(".set stderr, __stderrp\n");
		printf("# ------------------------------------------------------------ #\n");
	#endif
	printf("# end globals =============\n\n");

	/* 
	 * Don't ask me why this tidbit here is necessary.
 	 * I probably pasted it from an assembly tutorial. 
	 */
	printf(".section .text\n");

	/* 
	 * We deal with the procedures separately
	 * before doing any further work to prevent
	 * their code from being output within
	 * the code for "main"
	 * (this codegen directly and linearly barfs
	 * out code without any kind of backtracking,
	 * it's easy to do that when the assembler
	 * is the one taking care of forward label
	 * jumps and symbols and such)
	 */
	proc_ok = 1;
	deal_with_procs(tree);
	proc_ok = 0;	/* no further procedures shall be allowed */

	#ifndef MINGW_BUILD
		puts(".type ___mymemcpy, @function");
	#endif
	puts("___mymemcpy:");
	puts(".globl ___mymemcpy");
	puts("# set up stack space");
	puts("pushq %rbp");
	puts("movq %rsp, %rbp");
	puts("subq $144, %rsp");
	puts("");
	puts("# >>> compiled code for '___mymemcpy'");
	puts("_tco____mymemcpy:");
	puts("movq %rdi, -144(%rbp)");
	puts("movq %rsi, -152(%rbp)");
	puts("movq %rdx, -160(%rbp)");
	puts("movq -144(%rbp), %rsi");
	puts("movq %rsi, %rdi");
	puts("movq %rdi, -8(%rbp)");
	puts("MYMEMCPY_IL0: ");
	puts("movq -160(%rbp), %rsi");
	puts("decq -160(%rbp)");
	puts("movq $0, %rdi");
	puts("cmpq %rsi, %rdi");
	puts("je MYMEMCPY_IL1");
	puts("movq -144(%rbp), %rsi");
	puts("addl $1, -144(%rbp)");
	puts("movq %rsi, %rax");
	puts("movq -152(%rbp), %rsi");
	puts("addl $1, -152(%rbp)");
	puts("movq %rsi, %rbx");
	puts("movsbq (%rbx), %rbx");
	puts("movq %rbx, %rbx");
	puts("movb %bl, (%rax) #ptra");
	puts("jmp MYMEMCPY_IL0");
	puts("MYMEMCPY_IL1: ");
	puts("# return value");
	puts("movq -8(%rbp), %rsi");
	puts("movq %rsi, %rax # ret");
	puts("jmp _ret____mymemcpy");
	puts("");
	puts("# clean up stack and return");
	puts("_ret____mymemcpy:");
	puts("addq $168, %rsp");
	puts("movq %rbp, %rsp");
	puts("popq %rbp");
	puts("ret");

	/*
	 * Test/debug the type analyzer
	 */
	#ifdef DEBUG
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
 * Create jump tables for switch statements
 * (they go in .rodata at the top of the .S output)
 */
void create_jump_tables(exp_tree_t* tree)
{
	int* caselab;
	char *str;
	int count = 0;
	int i;
	int lab;
	int maxlab = -1;
	exp_tree_t decor;
	token_t num;

	/*
	 * Recurse down the code tree looking
	 * for a switch-statement tree node
	 */
	for (i = 0; i < tree->child_count; ++i) {
		create_jump_tables(tree->child[i]);
	}

	/*
	 * Found a switch statement node,
	 * so now do the actual work
	 */
	if (tree->head_type == SWITCH) {
		/*
		 * Memory allocation and initialization
		 * -- boring run-of-the-mill bureacratic stuff
		 */
		caselab = malloc(1024 * sizeof(int));
		if (!caselab)
			fail("couldn't malloc a few kilobytes :(");

		/*
		 * Look for case labels, and associate
		 * their index to their numeric labels
		 */
		for (i = 0; i < 1024; ++i)
			caselab[i] = -1;
		for (i = 0; i < tree->child_count; ++i) {
			if (tree->child[i]->head_type == SWITCH_CASE) {
				/*
				 * "switch(u) { case 0: foo(); }"
				 * 
				 * makes the tree:
				 * 
				 * (SWITCH:switch 
				 *	(VARIABLE:u)
				 *	(SWITCH_CASE:0)
				 * 	(PROC_CALL:foo))
				 */
				str = get_tok_str(*(tree->child[i]->tok));
				sscanf(str, "%d", &lab);
				/*
				 * XXX: a more sophisticated scheme
				 * may address these range issues, but I'm
				 * getting a basic version working for now.
				 */
				if (lab < 0) {
					compiler_fail("can't do negative case-values yet",
							findtok(tree), 0, 0);
				}
				if (lab > 1023) {
					compiler_fail("can't do case-values > 1023 yet",
							findtok(tree), 0, 0);
				}

				/*
				 * Figuring out which case label value is the 
				 * highest
				 */
				if (lab > maxlab)
					maxlab = lab;

				/*
				 * Associate this case label value to the index of
				 * this case label
				 */
				caselab[lab] = count++;
			}
		}

		/*
		 * Write out the jump table
		 */
		printf("########## jump table for `switch' %-4d #########\n", switch_count);
		printf("jt%d: .quad ", switch_count);
		fflush(stdout);
		for (i = 0; i <= maxlab; ++i) {
			if (i) {
				printf(", ");
				fflush(stdout);
			}
			if (caselab[i] == -1) {
				/* 
				 * no match: jump to `default' label (which is made to
				 * be the end of the switch if there is no `default' defined)
			 	 */
				printf("jt%d_def", switch_count);
			} else {
				printf("jt%d_l%d", switch_count, caselab[i]);
			}
			fflush(stdout);
		}
		printf("\n");
		printf("#################################################\n");

		/*
		 * keep track of maximum label.
		 * XXX: array limited to 256 entries
		 */
		switch_maxlab[switch_count] = maxlab;

		/*
		 * "decorate" the tree with the number
		 * of its jumptable pointer (given by `switch_count')
		 */
		str = malloc(64);
		if (!str)
			fail("i couldn't get my 64 bytes, i'm quite sad");
		sprintf(str, "%d", switch_count);
		num.start = str;
		num.len = strlen(str);
		num.from_line = 0;
		num.from_line = 1;
		decor = new_exp_tree(NUMBER, &num);
		add_child(tree, alloc_exptree(decor));

		/*
		 * clean up and increase counter
		 */
		free(caselab);
		++switch_count;
	}
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
		/* 
		 * Actually, I think the GNU assembler automatically
	 	 * does these
		 */
	}

	/* child recursion */
	for (i = 0; i < tree->child_count; ++i)
		deal_with_str_consts(tree->child[i]);
}


/* 
 * Code all the function definitions in a program
 * It also deals with prototypes now.
 */
void deal_with_procs(exp_tree_t *tree)
{
	int i;

	/* if this is a procedure-definition node */
	if (tree->head_type == PROC 
		|| tree->head_type == PROTOTYPE) {

		/* clear local symbols accounting */
		syms = 0;
		arg_syms = 0;

		/* code the arguments and body */
		codegen(tree);

		/* 
		 * Discard the func def tree, it's
		 * over with being coded now and nobody else
		 * wants to see it
		 */
		(*tree).head_type = NULL_TREE;
	}

	/* child recursion */
	for (i = 0; i < tree->child_count; ++i)
		if (tree->child[i]->head_type == BLOCK
		|| tree->child[i]->head_type == PROC
		|| tree->child[i]->head_type == PROTOTYPE)
			deal_with_procs(tree->child[i]);
}

/* 
 * Get a GAS (64-bit) X86 instruction suffix
 * from a type declaration code
 */
char *decl2suffix(char ty)
{
	switch (ty) {
		case LONG_DECL:		/* 8 bytes */
			return "q";
		case INT_DECL:		/* 8 bytes */
			return "q";
		case CHAR_DECL:		/* 1 byte */
			return "b";
		default:		/* ??? */
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
		case 8:
			return "q";
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
	return ty == CHAR_DECL || ty == INT_DECL || ty == LONG_DECL
		|| ty == VOID_DECL;
}

/* 
 * This routine populates the symbol table 
 * with the currently-being-coded function's 
 * local variables and sets aside
 * stack space for them. It is kind of
 * a messy process. In fact it is probably
 * one of the worst parts of this compiler.
 */
void setup_symbols(exp_tree_t *tree, int symty)
{
	extern void setup_symbols_iter(exp_tree_t *tree, int symty, int first_pass);
	setup_symbols_iter(tree, symty, 1);
	if (symty != SYMTYPE_GLOBALS) {
		setup_symbols_iter(tree, symty, 0);
	}
}

void setup_symbols_iter(exp_tree_t *tree, int symty, int first_pass)
{
	int override = symty == SYMTYPE_GLOBALS;
	int i, j;
	exp_tree_t *dc;
	int sym_num;
	int objsiz;
	int decl = tree->head_type;
	typedesc_t typedat, struct_base;
	typedesc_t array_base_type;
	struct_desc_t* sd;
	int struct_bytes;
	exp_tree_t inits;
	exp_tree_t init_expr;
	exp_tree_t *initializer_val;
	int children_offs;
	int is_extern = 0;
	char init_str[128];

	/*
	 * Externs are added to the global symbols list
	 */
	if (tree->head_type == EXTERN_DECL) {
		symty = SYMTYPE_GLOBALS;
		tree->head_type = NULL_TREE;
		tree = tree->child[0];
		decl = tree->head_type;
		is_extern = 1;
	}

	/*
	 * Handle structs in the second pass
	 * (somehow it calms down the segfault gods
	 * to set up their stack after that of the other 
	 * types of variables. I'm clueless about the
	 * finer details of x86)
	 */
	if ((tree->head_type == STRUCT_DECL
		|| tree->head_type == NAMED_STRUCT_DECL) && ((override||!first_pass) || is_extern)) {

		/*
		 * The first part of handling a struct declaration (e.g.
		 * "struct meal *hamburger = NULL") is (unsurprisingly enough)
		 * handling the actual "struct" definition
		 * part. This could either be the full thing, e.g.
		 * "struct meal { char *drink; char *main_plate; }"
		 * or a named reference, e.g. "struct meal".
		 */

		/* the full thing */
		if (tree->head_type == STRUCT_DECL) {
			#ifdef DEBUG
				fprintf(stderr, "struct variable with base struct %s...\n", 
					get_tok_str(*(tree->tok)));
			#endif
			/* parse the struct declaration syntax tree 
			 * (into a type descriptor structure) */
			struct_base = struct_tree_2_typedesc(tree, &struct_bytes, &sd);
			struct_base.struct_desc->bytes = struct_bytes;

			/* 
			 * Register the struct's name (e.g. "bob" in 
			 * "struct bob { ... };"), because it might
			 * be referred to simply by its name later on
			 * (in fact that's what the else-clause below 
			 * deals with)
			 * 
			 * XXX: wait, what if it's anonymous ?
			 */
			named_struct[named_structs] = struct_base.struct_desc;
			strcpy(named_struct_name[named_structs],
				get_tok_str(*(tree->tok)));
			++named_structs;
			expandBuffers();
			children_offs = sd->cc;
		} else {
			/* NAMED_STRUCT_DECL -- named reference case */
			struct_base.ty = 0;		
			struct_base.arr = struct_base.ptr = 0;
			struct_base.is_struct = 1;
			struct_base.is_struct_name_ref = 0;
			sd = struct_base.struct_desc = 
				find_named_struct_desc(get_tok_str(*(tree->tok)));

			children_offs = 0;
			struct_bytes = struct_base.struct_desc->bytes;
		}

		/* Discard the tree from further codegeneneration */
		tree->head_type = NULL_TREE;

		/*
		 * Create initializers tree
		 * (initializers are actual code, so they must
		 * be packaged together in an artificial syntax tree that
		 * gets sent off to the main codegen routine.
		 * The current part of the codegen doesn't do actual code,
		 * it does symbol table, typing, and memory accounting stuff)
		 */
		inits = new_exp_tree(BLOCK, NULL);

		/* 
		 * Now do the declaration children (i.e. the actual
		 * variables, with pointer/array modifiers and 
		 * symbol name identifiers, after the "struct {... } " part,
	 	 * e.g. the "*ronald[32]" part in 
		 * "struct corporate_mascot_instance *ronald[32]".
		 * it's a loop, because there could be several of them,
		 * (each with their own particular array and pointer qualifications,
		 * as in e.g. "struct clown **bozo, donald, *gerald[32]")
		 */
		for (i = children_offs; i < tree->child_count; ++i) {
			dc = tree->child[i];

			/* 
			 * The type of the declaration child is derived
			 * from the struct type information (`struct_base')
			 * by adding pointer or array qualifications
			 */

			typedat = struct_base;

			/* array qualifications */
			parse_array_info(&typedat, dc);

			/* pointer qualifications */
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
			 * code syntax tree (that weird contrived thing that I
			 * created a few paragraphs ago and justified the creation
			 * of)
			 */
			if (dc->child_count - typedat.arr - typedat.ptr - 1) {
				for (j = 1; j < dc->child_count; ++j)
					/* 
					 * If a declaration tree's node encountered at
					 * this depth is not an array or pointer qualifier,
					 * it's automatically an initializer. it could
					 * be of several types (number, variable, 
					 * array literal, whatever), so that's why 
					 * this check is a "negative" or "not" check 
					 */
					if (!(dc->child[j]->head_type == ARRAY_DIM
						|| dc->child[j]->head_type == DECL_STAR)) {
						initializer_val = dc->child[j];
						break;
					}

				/* 
				 * Synthesize an assignment node
				 * (so yeah, initializations are basically 
				 * delayed assignments)
				 */
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
				/* 
				 * If it's any kind of pointer, it's easy,
				 * it's always 8 bytes on 64-bit x86
				 */
				objsiz = 8;
			else
				/* 
				 * Not a pointer: it's just the size of the struct
				 * (remember that right now we are dealing exclusively
				 * with struct declarations)
				 */
				objsiz = struct_bytes;

			if (typedat.arr) {
				/* 
				 * N-dimensional array of struct (or of N-degree 
				 * pointers to struct)
				 */
				array_base_type = typedat;
				array_base_type.arr = 0;

				/*
				 * There's some arithmetic computation involved in creating
				 * array symbols, and create_array() deals with that.
				 * Specifically it deals with the fact that an n-element array of 
				 * objects of type X is n times larger in memory than a plain-ass object of 
				 * type X. To make things nastier, n needs to be computed as a product
				 * of dimensions if the array is multi-dimensional:
				 * e.g. if you have int matrix[3][2], n would be 6.
				 * All that trouble is encapsulated in that routine.
				 * And it also deals with setting up the memory space and symbols
				 * and whatnot.
				 */
				sym_num = create_array(symty, dc,
					type2siz(array_base_type), arr_dim_prod(typedat) * objsiz,
					is_extern);
			} else {
				/*
				 * It's just a non-arrayed variable (either a struct
				 * or an N-degree pointer thereof); just add it to
				 * the appropriate symbol table.
				 */
				if (symty == SYMTYPE_GLOBALS) {
					if (!is_extern) {
						/* register as global (needed for multifile) */
						printf(".globl %s\n", 
							get_tok_str(*(dc->child[0]->tok)));
						printf(".comm %s,%d,32\n",
							get_tok_str(*(dc->child[0]->tok)), objsiz);
					}
					sym_num = glob_add(dc->child[0]->tok);
				}
				else
					sym_num = sym_add(dc->child[0]->tok, objsiz);
			}

			/* 
			 * Add type data to appropriate symbol table
			 * (above we were taking care just of the name,
			 * and actually also the size, but not the type.
			 * The type is potentially supercrucial!)
			 */
			if (symty == SYMTYPE_GLOBALS) {
				globtyp[sym_num] = typedat;
				if (is_extern) {
					globtyp[sym_num].is_extern = 1;
				}
			}
			else
				symtyp[sym_num] = typedat;
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
	 * (thankfully this is not as bad as dealing with structs)
	 */
	if (int_type_decl(decl)) {
		/* 
		 * As earlier, we loop over the declaration children,
		 * i.e. e.g. {a, b, c} in "int a, b, c;"
		 */
		for (i = 0; i < tree->child_count; ++i) {
			dc = tree->child[i];

			/* 
			 * Do all the smaller stuff first;
			 * for some reason, if I put ints at
			 * big offsets like -2408(%rbp),
			 * segfaults happen. (???)
			 * pleasing the incomprehensible 
			 * segfault gods again, yes
			 * 
			 * Maybe it's actually an alignment
			 * issue, but googling gives nothing
			 * and I'm not smart and brave enough
			 * to spend 3 hours reading Intel PDFs.
			 * And anyway this is just a toy project,
			 * innit ?
			 */
			if (!is_extern) {
				if ((!override && first_pass) && check_array(dc))
					continue;
				if ((!override && !first_pass) && !check_array(dc))
					continue;
			}

			/* 
			 * Get some type information about the object
			 * declared
			 */
			parse_type(dc, &typedat, &array_base_type,
				&objsiz, decl, NULL);
			discard_stars(dc);

			/* 
			 * Switch symbols/stack setup code
			 * based on array dimensions of object
			 */
			if (check_array(dc) > 0) {
				/* Create the memory and symbols */
				sym_num = create_array(symty, dc,
					type2siz(array_base_type), objsiz, is_extern);

				/* Discard the tree if it has no initializer */
				if (dc->child_count - 1 - check_array(dc) == 0)
					dc->head_type = NULL_TREE;
				else {
					/* Otherwise make the initializer into an assignment node */
					initializer_val = alloc_exptree(
						new_exp_tree(COMPLICATED_INITIALIZATION, NULL));
					for (j = 0; j < dc->child_count; ++j)
						if (dc->child[j]->head_type != ARRAY_DIM)
							add_child(initializer_val, dc->child[j]);
					memcpy(dc, initializer_val, sizeof(exp_tree_t));
				}
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
						if (is_extern || !glob_check(dc->child[0]->tok)) {
							/* 
							 * Fold constant expressions in initializer
							 */
							if (dc->child_count == 2)
								constfold(dc->child[1]);

							/*
							 * If there is an initializer, make sure
							 * it is an integer constant.
							 */
							if (dc->child_count == 2
								&& !get_global_initializer_string_gnux86(dc->child[1], init_str)) {
								codegen_fail("invalid or unsupported global initializer",
									dc->child[0]->tok);
							}

							/* 
							 * Make the symbol and write out the label
							 */
							sym_num = glob_add(dc->child[0]->tok);

							if (is_extern)
								break;

							printf("%s: ",
								get_tok_str(*(dc->child[0]->tok)));

							/* 
							 * If no initial value is given,
							 * write a zero, otherwise write
							 * the initial value.
							 */
							/* XXX: change .long for types other than `int' */

							if (dc->child_count == 2) {
								printf(".long %s\n", init_str);
							} else {
								printf(".long 0\n");
							}

							printf(".globl %s\n\n",
								get_tok_str(*(dc->child[0]->tok)));
						}
						break;
					default:
						fail("codegen_amd64 internal messup");
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
				if (is_extern) {
					globtyp[sym_num].is_extern = 1;
				}
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
		if (!override && !first_pass)
			tree->head_type = BLOCK;
	}

	/* Child recursion, with some special provisions
	 * to ensure some things happen only in their assigned pass */
	for (i = 0; i < tree->child_count; ++i)
		if (tree->child[i]->head_type == BLOCK
		||  tree->child[i]->head_type == IF
		||  tree->child[i]->head_type == WHILE
		||  tree->child[i]->head_type == EXTERN_DECL
		||  int_type_decl(tree->child[i]->head_type)
		||  ((override || !first_pass) && tree->child[i]->head_type == STRUCT_DECL)
		||  ((override || !first_pass) && tree->child[i]->head_type == NAMED_STRUCT_DECL))
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
	printf("movq %s, %s\n", stor, str);
	return str;
}

char* registerize_freemem(char *stor)
{
	char *reg = registerize(stor);
	free_temp_mem(stor);
	return reg;
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
		fixreg(stor, fromsiz), str);
	return str;
}

/*
 * Get a register that is allowed to be used
 * in operations of a certain size
 * (all this because you apparently can't
 * do byte-level work with ESI and EDI.
 * don't you love microprocessors ?)
 */
char* registerize_siz(char *stor, int siz)
{
	int is_reg = 0;
	int i;
	char* str;

	/* Error: `%rsi' not allowed with `movb' */
	/* Error: `%rdi' not allowed with `movb' */
	if (siz == 1 
		&& (!strcmp(stor, "%rsi") || !strcmp(stor, "%rdi") ||!strcmp(stor, "%rbx"))) {
		free_temp_reg(stor);
	}
	else
		for (i = 0; i < TEMP_REGISTERS; ++i)
			if (!strcmp(stor, temp_reg[i]))
				return stor;
	
	str = get_temp_reg_siz(siz);
	printf("movq %s, %s\n", stor, str);
	return str;
}

/*
 * The core codegen routine. It returns
 * the temporary register at which the runtime
 * evaluation value of the tree is stored
 * (if the tree is an expression
 * with a value). It's actually one of the
 * neater parts of the program. It's a fairly
 * clean tree recursion that does a case analysis
 * on the type of the current node, and barfs out
 * the precious x86 assembler code along the way. 
 * In a better world this would be a switch() statement,
 * I guess.
 *
 * Several people have told me that this routine is too long.
 * To this, I answer that careful inspection reveals
 * that it consists in a chain of several mutually-exclusive
 * if-statements, which are each not much longer than about
 * 100 lines, and which each deal with one specific 
 * syntax-tree head-type. Furthermore, it is not easily possible
 * to massage this into a different code structure (e.g. a 
 * switch statement) since in some cases the if statements 
 * have several &&'d conditions as their predicate.
 */
char* codegen(exp_tree_t* tree)
{
	char *sto, *sto2, *sto3;
	char *str, *str2;
	char *name;
	char *proc_args[32];
	char my_ts_used[TEMP_REGISTERS];
	char *buf;
	char sbuf[256], sbuf2[256];
	int i;
	char *sym_s;
	char *oper;
	char *arith;
	token_t one;
	token_t fakenum;
	token_t fakenum2;
	token_t faketok;
	exp_tree_t one_tree;
	exp_tree_t fake_tree;
	exp_tree_t fake_tree_2;
	exp_tree_t fake_tree_3;
	exp_tree_t fake_tree_4;
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
	int do_deref;
	int tlab;
	int entries, fill;
	int jumptab;
	int casenum;
	int defset;
	int offs;

	/*
		token_t one = { TOK_INTEGER, "1", 1, 0, 0 };
		token_t fakenum = { TOK_INTEGER, "0", 1, 0, 0 };
		token_t fakenum2 = { TOK_INTEGER, "0", 1, 0, 0 };
	*/
	one.type = TOK_INTEGER;
	fakenum.type = TOK_INTEGER;
	fakenum2.type = TOK_INTEGER;
	one.start = my_strdup("1");
	one.len = 1;
	fakenum.start = my_strdup("0");
	fakenum.len = 1;
	fakenum2.start = my_strdup("0");
	fakenum2.len = 1;
	one_tree = new_exp_tree(NUMBER, &one);

	/*
	 * Track the tree currently
	 * being coded for error-message
	 * purposes
	 */
	codegen_current_tree = tree;

/*
	#ifdef DEBUG
		if (findtok(tree))
			fprintf(stderr, "trying to compile line %d\n",
				findtok(tree)->from_line);
	#endif
*/

	/* 
	 * New statement (IF, etc) or compound statement (BLOCK);
	 * clear temporary memory and registers used to evaluate
	 * expressions
	 */
	if (tree->head_type == BLOCK
		|| tree->head_type == IF
		|| tree->head_type == WHILE
		|| tree->head_type == BPF_INSTR) {
		new_temp_mem();
		new_temp_reg();
	}


	#ifdef MINGW_BUILD
	/*
	 * The strange MINGW symbols for stdin, stdeer, and stdout,
	 * based on the output of mingw gcc 4.6.2 in -S mode
	 */
	if (tree->head_type == MINGW_IOBUF
		&& !strcmp(get_tok_str(*(tree->tok)), "stdin")) {
		sto = get_temp_reg();
		printf("movq __imp___iob, %s\n", sto);
		return sto;
	}
	if (tree->head_type == MINGW_IOBUF
		&& !strcmp(get_tok_str(*(tree->tok)), "stdout")) {
		sto = get_temp_reg();
		printf("movq __imp___iob, %s\n", sto);
		printf("addq $32, %s\n", sto);
		return sto;
	}
	if (tree->head_type == MINGW_IOBUF
		&& !strcmp(get_tok_str(*(tree->tok)), "stderr")) {
		sto = get_temp_reg();
		printf("movq __imp___iob, %s\n", sto);
		printf("addq $64, %s\n", sto);
		return sto;
	}
	#endif

	/* comma-separated sequence */
	if (tree->head_type == SEQ) {
		for (i = 0; i < tree->child_count; ++i) {
			sto = codegen(tree->child[i]);
			if (i < tree->child_count - 1) {
				free_temp_reg(sto);
				free_temp_mem(sto);
			}
		}
		return sto;
	}

	/* code complicated initializations */
	if (tree->head_type == COMPLICATED_INITIALIZATION) {
		#ifdef DEBUG
			printout_tree(*tree);
			fprintf(stderr, "\n");
		#endif
		/* 
		 * string constant initializer
		 * 
	 	 * e.g. char bob[1024] = "hahaha";
		 * (COMPLICATED_INITIALIZATION (VARIABLE:bob) (STR_CONST:"hahaha"))
		 */
		if (tree->child[1]->head_type == STR_CONST
			&& tree_typeof(tree->child[0]).arr == 1) {
			for (i = 0; i < tree->child[1]->tok->len - 1; ++i) {
				sprintf(sbuf, "%d", i);
				fakenum.start = &sbuf[0];
				fakenum.len = strlen(sbuf);
				if (i == tree->child[1]->tok->len - 2)
					sprintf(sbuf2, "0");	/* null terminator */
				else
					sprintf(sbuf2, "%d", tree->child[1]->tok->start[i + 1]);
				/*
			 	 * We artificially create a bunch of assignment
				 * statement parse trees, bob[0] = 'h', bob[1] = 'e', etc.
				 * and codegen them one-by-one.
				 */
				fakenum2.start = &sbuf2[0];
				fakenum2.len = strlen(sbuf2);
				fake_tree_3 = new_exp_tree(NUMBER, &fakenum);
				fake_tree_4 = new_exp_tree(NUMBER, &fakenum2);
				fake_tree = new_exp_tree(ASGN, NULL);
				fake_tree_2 = new_exp_tree(ARRAY, NULL);
				add_child(&fake_tree_2, tree->child[0]);
				add_child(&fake_tree_2, alloc_exptree(fake_tree_3));
				add_child(&fake_tree, alloc_exptree(fake_tree_2));
				add_child(&fake_tree, alloc_exptree(fake_tree_4));
				#ifdef DEBUG
					printout_tree(fake_tree);
					fprintf(stderr, "\n");
				#endif
				(void)codegen(&fake_tree);
				new_temp_mem();
				new_temp_reg();
			}
			return NULL;
		}
		/*
		 * array initializer
		 * 
		 * e.g. int arr[] = {1, 2, 3};
		 * (COMPLICATED_INITIALIZATION 
		 *	(VARIABLE:arr) 
		 *	(COMPLICATED_INITIALIZER (NUMBER:1) (NUMBER:2) (NUMBER:3)))
		 */
		else if (tree_typeof(tree->child[0]).arr == 1) {
			if (tree_typeof(tree->child[0]).arr_dim[0] > 1
				&& tree->child[1]->child_count == 1) {
				/* fill -- e.g. int arr3[10] = {0}; */
				#ifdef DEBUG
					fprintf(stderr, "fill initializer\n");
				#endif
				fill = 1;
				entries = tree_typeof(tree->child[0]).arr_dim[0];
			}
			else {
				fill = 0;
				entries = tree->child[1]->child_count;
			}
			for (i = 0; i < entries; ++i) {
				/* (ASGN (ARRAY (VARIABLE:arr) (NUMBER:i)) XXX) */	
				sprintf(sbuf, "%d", i);
				fakenum.start = &sbuf[0];
				fakenum.len = strlen(sbuf);
				fake_tree_3 = new_exp_tree(NUMBER, &fakenum);
				fake_tree = new_exp_tree(ASGN, NULL);
				fake_tree_2 = new_exp_tree(ARRAY, NULL);
				add_child(&fake_tree_2, tree->child[0]);
				add_child(&fake_tree_2, alloc_exptree(fake_tree_3));
				add_child(&fake_tree, alloc_exptree(fake_tree_2));
				if (fill)
					add_child(&fake_tree, tree->child[1]->child[0]);
				else
					add_child(&fake_tree, tree->child[1]->child[i]);	
				#ifdef DEBUG
					printout_tree(fake_tree);
					fprintf(stderr, "\n");
				#endif
				(void)codegen(&fake_tree);
				new_temp_mem();
				new_temp_reg();
			}
			return NULL;
		}
	}

	/* ternary operator -- ? : */
	if (tree->head_type == TERNARY) {
		tlab = ++intl_label;
		intl_label++;
		intl_label++;
		intl_label++;
		sto = registerize_siz(codegen(tree->child[0]), 8);
		printf("cmpq $0, %s\n", sto);
		free_temp_reg(sto);
		printf("je IL%d\n", tlab);
		sto = registerize_siz(codegen(tree->child[1]), 8);
		printf("jmp IL%d\n", tlab + 1);
		printf("IL%d:\n", tlab);
		free_temp_reg(sto);
		sto = registerize_siz(codegen(tree->child[2]), 8);
		printf("IL%d:\n", tlab + 1);
		return(sto);
	}

	/* 
	 * -> read
	 */
	if (tree->head_type == DEREF_STRUCT_MEMB) {
		strcpy(sbuf, get_tok_str(*(tree->child[1]->tok)));
		offs = struct_tag_offs(tree_typeof(tree->child[0]),
						sbuf);

		printf("# -> load tag `%s' with offset %d\n", sbuf, offs);

		membsiz = type2siz(tree_typeof(tree));

		/* get base adr */
		if (tree->child[0]->head_type != VARIABLE) {
			if (tree->child[0]->head_type == ADDR
				&& check_proc_call(tree->child[0]->child[0])) {
				/*
				 * struct-returning procedures are
				 * coded as actually returning pointers,
				 * so disregard the ADDR node.
				 */
				sym_s = registerize(codegen(tree->child[0]->child[0]));
			} else {
				sym_s = registerize(codegen(tree->child[0]));
			}
		} else {
			sym_s = sym_lookup(tree->child[0]->tok);
		}

		sto2 = get_temp_reg();

		/* build pointer */
		printf("movq %s, %s\n", sym_s, sto2);
		free_temp_reg(sym_s);
		printf("addq $%d, %s\n", offs, sto2);
	
		/* 
		 * Yet another subtlety...
		 * if the tag is an array,
		 * do not dereference, because
		 * arrays evaluate to pointers at run-time.
		 * Also, for large struct-members, we return
		 * a pointer, by convention.
		 */
		if (tree_typeof(tree).arr || membsiz > 8) {
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
		strcpy(sbuf, get_tok_str(*(tree->child[0]->child[1]->tok)));
		offs = struct_tag_offs(tree_typeof(tree->child[0]->child[0]),
						sbuf);

		printf("# load address of tag `%s' with offset %d\n", sbuf, offs);

		if (type2siz(tree_typeof(tree)) > 8) {
			/* 
			 * Rewrite large assignments as:
			 *
			 * (PROC_CALL:___mymemcpy 
			 *     (VARIABLE:b)
			 *     (ADDR (VARIABLE:a))
			 *     (SIZEOF (VARIABLE:a)))
			 *
			 * Where ___mymemcpy is a custom routine
			 * autoincluded in the compiled output.
			 * This should deal with struct assignments.
			 */

			faketok.type = TOK_IDENT;
			buf = newstr("___mymemcpy");
			faketok.start = buf;
			faketok.len = strlen(buf);
			faketok.from_line = 0;
			faketok.from_line = 1;

			fake_tree = new_exp_tree(PROC_CALL_MEMCPY, &faketok);
			if (tree->child[1]->head_type == DEREF
				|| tree->child[1]->head_type == DEREF_STRUCT_MEMB) {
				fake_tree_3 = *(tree->child[1]->child[0]);
			} else if (check_proc_call(tree->child[1]) || tree->child[1]->head_type == ARRAY) {
				fake_tree_3 = *(tree->child[1]);
			} else {
				fake_tree_3 = new_exp_tree(ADDR, NULL);
				add_child(&fake_tree_3, tree->child[1]);
			}
			fake_tree_4 = new_exp_tree(SIZEOF, NULL);
			add_child(&fake_tree_4, tree->child[0]);
			add_child(&fake_tree, tree->child[0]->child[0]);
			add_child(&fake_tree, alloc_exptree(fake_tree_3));
			add_child(&fake_tree, alloc_exptree(fake_tree_4));
			return codegen(alloc_exptree(fake_tree));
		}

		/* get base adr */
		if (tree->child[0]->child[0]->head_type != VARIABLE)
			sym_s = registerize(codegen(tree->child[0]->child[0]));
		else
			sym_s = sym_lookup(tree->child[0]->child[0]->tok);

		sto2 = get_temp_reg();

		/* build pointer */
		printf("movq %s, %s\n", sym_s, sto2);
		free_temp_reg(sym_s);
		printf("addq $%d, %s\n", offs, sto2);
	
		sto3 = registerize(codegen(tree->child[1]));
		printf("movq %s, (%s)\n", sto3, sto2);
		free_temp_reg(sto2);
		return sto3;
	}

	/*
	 * &(a->b)
	 */
	if (tree->head_type == ADDR
		&& tree->child[0]->head_type == DEREF_STRUCT_MEMB) {
		tree = tree->child[0];

		strcpy(sbuf, get_tok_str(*(tree->child[1]->tok)));
		offs = struct_tag_offs(tree_typeof(tree->child[0]),
						sbuf);

		printf("# -> load tag `%s' with offset %d\n", sbuf, offs);

		membsiz = type2siz(tree_typeof(tree));

		/* get base adr */
		if (tree->child[0]->head_type != VARIABLE)
			sym_s = registerize(codegen(tree->child[0]));
		else
			sym_s = sym_lookup(tree->child[0]->tok);

		sto2 = get_temp_reg();

		/* build pointer */
		printf("movq %s, %s\n", sym_s, sto2);
		free_temp_reg(sym_s);
		printf("addq $%d, %s\n", offs, sto2);
		
		return sto2;
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

	/* array variable load */
	if (tree->head_type == VARIABLE
		&& sym_lookup_type(tree->tok).arr) {
		/*
		 * When arrays are evaluated, the result is a
		 * pointer to their first element (see earlier
		 * comments about the paper called 
		 * "The Development of the C Language"
		 * and what it says about this)
		 */

		/* build pointer to first member of array */
		sto = get_temp_reg_siz(8);
		printf("leaq %s, %s\n",
			sym_lookup(tree->tok),
			sto);
		return sto;
	}

	/* string constant, e.g. "bob123" */
	if (tree->head_type == STR_CONST) {
		sto = get_temp_reg_siz(8);
		printf("movq $_str_const_%d, %s\n",
			str_const_lookup(tree->tok),
			sto);
		return sto;
	}

	/* &*x = x */
	if (tree->head_type == ADDR
		&& tree->child_count == 1
		&& tree->child[0]->head_type == DEREF) {
		return codegen(tree->child[0]->child[0]);
	}

	/* address-of variable -- &x */
	if (tree->head_type == ADDR
		&& tree->child_count == 1
		&& tree->child[0]->head_type == VARIABLE) {
		
		sto = get_temp_reg_siz(8);

		/* LEA: load effective address */
		printf("leaq %s, %s\n",
				sym_lookup(tree->child[0]->tok),
				sto);

		return sto;
	}

	/* address-of assigned variable -- &(a = b) */
	if (tree->head_type == ADDR
		&& tree->child_count == 1
		&& tree->child[0]->head_type == ASGN
		&& tree->child[0]->child[0]->head_type == VARIABLE) {
		
		/* code the assignment */
		codegen(tree->child[0]);

		sto = get_temp_reg_siz(8);

		/* LEA: load effective address */
		printf("leaq %s, %s\n",
				sym_lookup(tree->child[0]->child[0]->tok),
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
			printf("imulq $%d, %s\n", offs_siz, sto2);

		/* ptr = base_adr + membsiz * index_expr */
		printf("addq %s, %s\n", sto2, sto);
		free_temp_reg(sto2);

		return sto;
	}

	/* dereference -- *(exp) */
	if (tree->head_type == DEREF
		&& tree->child_count == 1) {
		/* 
		 * Find byte size of *exp
		 */
		membsiz = type2siz(
			deref_typeof(tree_typeof(tree->child[0])));
		
		sto = registerize_siz(codegen(tree->child[0]), membsiz);

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
			sto, sto);

		return sto;
	}

	/* procedure call */
	if (tree->head_type == PROC_CALL 
		|| tree->head_type == PROC_CALL_MEMCPY) {


		char *arg_temp_mem[32];
		int arg_temp_mem_idx;

		memcpy(my_ts_used, ts_used, TEMP_REGISTERS);
		
		/* 
		 * Push all the temporary registers
		 * that are currently in use
		 */
		printf("subq $%d, %%rsp\n", TEMP_REGISTERS * 8);
		for (i = 0; i < TEMP_REGISTERS; ++i)
			if (1 ||ts_used[i])
				printf("pushq %s\t# save temp register\n",
					temp_reg[i]);

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
			if (tree->head_type != PROC_CALL_MEMCPY) {
				compiler_warn("no declaration found, "
					      "assuming all arguments are 64-bit int",
					findtok(tree), 0, 0);
			}
			/* sizeof(int) = 8 on 64-bit x86 */
			callee_argbytes = tree->child_count * 8;
		}
		else {
			/* sum individual offsets */
			callee_argbytes = 0;
			for (i = 0; i < tree->child_count; ++i)
				callee_argbytes += type2siz(callee_argtyp[i]);
		}

		/* 
		 * Move the arguments into the appropriate registers (AMD64 special)
		 */
		offset = 0;
		for (i = 0; i < tree->child_count; ++i) {
			/* Find out the type of the current argument */
			if (!callee_argtyp) {
				/* 
				 * Default to int if no declaration 
				 * provided
				 */
				typedat.ty = INT_DECL;
				typedat.ptr = typedat.arr = 0;
				typedat.is_struct = 0;
				typedat.is_struct_name_ref = 0;

				/* offset = sizeof(int) * i */
				offset = 8 * i;
			} else {
				/* load registered declaration */
				typedat = callee_argtyp[i];
			}

			/* Get byte size of current argument */
			membsiz = type2siz(typedat);

			/* 
			 * XXX: might have to convert args to 
			 * function's arg type
			 */
			if (membsiz > 8) { 
				/*
				 * XXX this needs to be redone properly for AMD64
 				 */

			} else {
				sto = registerize(codegen(tree->child[i]));
				arg_temp_mem[i] = get_temp_mem();
				printf("movq %s, %s\t",
					sto, arg_temp_mem[i]);
				printf("# argument %d to %s\n",
					i, get_tok_str(*(tree->tok)));
				free_temp_reg(sto);
			}

			if (callee_argtyp)
				offset += type2siz(callee_argtyp[i]);
		}

		/*
		 * Mystery AMD64 hax. If I don't do this printf calls crash.
		 * I think it has something to do with varargs handling...
		 */
		printf("movl $0, %%eax\n");
	
		/* 
		 * Move the argument values held in temporary stack to the 
		 * final calling registers 
		 */
		for (i = 0; i < tree->child_count; ++i) {
			printf("movq %s, %s\n", arg_temp_mem[i], amd64_calling_registers[i]);
		}

		/* 
		 * Call the subroutine
		 */
		#ifdef MINGW_BUILD
			if (!strcmp(get_tok_str(*(tree->tok)), "___mymemcpy")) {
				printf("call %s\n", get_tok_str(*(tree->tok)));
			} else {
				printf("call _%s\n", get_tok_str(*(tree->tok)));
			}
		#else
			printf("call %s\n", get_tok_str(*(tree->tok)));
		#endif

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
		printf("movq %%rax, %s\n", sto);

		/* 
		 * Restore the registers as they were
		 * prior to the call -- pop them in 
		 * reverse order they were pushed
		 */
		for (i = 0; i < TEMP_REGISTERS; ++i)
			if (1 ||my_ts_used[TEMP_REGISTERS - (i + 1)])
				printf("popq %s\t# restore temp register\n",
					temp_reg[TEMP_REGISTERS - (i + 1)]);

		printf("addq $%d, %%rsp\n", TEMP_REGISTERS * 8);

		memcpy(ts_used, my_ts_used, TEMP_REGISTERS);

		/* 
		 * Give back the temporary stack storage
		 * with the return value in it
		 */
		return sto;
	}

	/* 
	 * This deals with a prototype.
	 * XXX: check if a proc def conflicts with an
	 * existing prototype
	 */
	if (tree->head_type == PROTOTYPE) {
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

		/*
		 * Special case: (void) means exactly zero arguments
		 */
		if (argl->child_count == 1
			&& argl->child[0]->child_count == 1
			&& argl->child[0]->child[0]->child_count == 1
			&& argl->child[0]->child[0]->child[0]->child_count == 1
			&& argl->child[0]->child[0]->child[0]->child[0]->head_type == VOID_DECL)
			argl->child_count = 0;

		argbytes = 0;

		expandBuffers();

		if (argl->child_count) {
			for (i = 0; argl->child[i]; ++i) {
				/* 
				 * Copy this argument's variable name to proc_args[i],
				 * unless it's a prototype, because prototypes may not
				 * have identifier names in all arguments. (as in e.g.
				 * "int donald(int, char*)")
				 */
				if (tree->head_type == PROC) {
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
				}

				/* Obtain and store argument type data */
				proto_argtyp[i] = tree_typeof(argl->child[i]);

				/* If no type specified, default to int */
				if (proto_argtyp[i].ty == TO_UNK)
					proto_argtyp[i].ty = INT_DECL;

				/* Calculate byte offsets */
				proto_argsiz[i] = type2siz(argtyp[i]);
				proto_argbytes += argsiz[i];

			}
			proc_args[i] = NULL;
		} else
			*proc_args = NULL;

		/* 
		 * Register function typing info
		 */

		#ifdef DEBUG
			fprintf(stderr, "Registering function `%s'...\n", get_tok_str(*(tree->tok)));
			fprintf(stderr, "Its return type description is: \n");
			dump_td(tree_typeof(tree));
		#endif

		/* argument types */
		for (i = 0; i < argl->child_count; ++i)
			func_desc[funcdefs].argtyp[i] = proto_argtyp[i];
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

		/*
		 * Special case: prototypes like foo(void)
		 * mean that the function takes exactly zero
		 * arguments.
		 */
		if (func_desc[funcdefs].argc == 1
		    && func_desc[funcdefs].argtyp[0].arr == 0
		    && func_desc[funcdefs].argtyp[0].ptr == 0
		    && func_desc[funcdefs].argtyp[0].is_struct == 0
		    && func_desc[funcdefs].argtyp[0].is_struct_name_ref == 0
		    && func_desc[funcdefs].argtyp[0].ty == VOID_DECL) {
			func_desc[funcdefs].argc = 0;
		}

		funcdefs++;

		/*
		 * expand buffer if necessary
		 */
		if (funcdefs >= func_desc_a) {
			while (funcdefs >= func_desc_a)
				func_desc_a *= 2;
			func_desc = realloc(func_desc, func_desc_a * sizeof(func_desc_t));
		}

		return NULL;
	}

	/* 
	 * This deals with a procedure definition
	 */
	if (tree->head_type == PROC) {
		/*
		 * Can't nest procedure definitions, not in C, anyway
		 */
		if (tree->head_type == PROC && !proc_ok)
			compiler_fail("function definition not allowed here (it's nested)",
				findtok(tree), 0, 0);

		/* 
		 * Put the list of arguments in char *proc_args,
		 * and also populate the argument type descriptions table
		 */
		custom_return_type = 0;
		if (tree->child[0]->head_type == CAST_TYPE 
		    || tree->child[0]->head_type == STRUCT_DECL) {
			custom_return_type = 1;
			argl = tree->child[1];
			compiler_warn("only int- or char- sized return types work",
					findtok(tree),
					0, 0);
		}
		else
			argl = tree->child[0];

		/*
		 * Special case: (void) means exactly zero arguments
		 */
		if (argl->child_count == 1
			&& argl->child[0]->child_count == 1
			&& argl->child[0]->child[0]->child_count == 1
			&& argl->child[0]->child[0]->child[0]->child_count == 1
			&& argl->child[0]->child[0]->child[0]->child[0]->head_type == VOID_DECL)
			argl->child_count = 0;

		argbytes = 0;

		expandBuffers();

		if (argl->child_count) {
			for (i = 0; argl->child[i]; ++i) {
				/* 
				 * Copy this argument's variable name to proc_args[i],
				 * unless it's a prototype, because prototypes may not
				 * have identifier names in all arguments. (as in e.g.
				 * "int donald(int, char*)")
				 */
				if (tree->head_type == PROC) {
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
				}

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

		#ifdef DEBUG
			fprintf(stderr, "Registering function `%s'...\n", get_tok_str(*(tree->tok)));
			fprintf(stderr, "Its return type description is: \n");
			dump_td(tree_typeof(tree));
		#endif

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

		/*
		 * Special case: prototypes like foo(void)
		 * mean that the function takes exactly zero
		 * arguments.
		 */
		if (func_desc[funcdefs].argc == 1
		    && func_desc[funcdefs].argtyp[0].arr == 0
		    && func_desc[funcdefs].argtyp[0].ptr == 0
		    && func_desc[funcdefs].argtyp[0].is_struct == 0
		    && func_desc[funcdefs].argtyp[0].is_struct_name_ref == 0
		    && func_desc[funcdefs].argtyp[0].ty == VOID_DECL) {
			func_desc[funcdefs].argc = 0;
		}

		funcdefs++;

		/*
		 * expand buffer if necessary
		 */
		if (funcdefs >= func_desc_a) {
			while (funcdefs >= func_desc_a)
				func_desc_a *= 2;
			func_desc = realloc(func_desc, func_desc_a * sizeof(func_desc_t));
		}

		/* prevent nested procedure defs */
		if (tree->head_type == PROC)
			proc_ok = 0;

		/*
		 * We fuck off early when it's a prototype,
	 	 * avoiding the block-coding part that only
		 * makes sense for actual procedure definitions
		 */
		if (tree->head_type == PROTOTYPE)
			return NULL;

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
			proc_args,
			argl);
		free(buf);

		/* okay you can define procedures again now */
		proc_ok = 1;

		return NULL;
	}

	/* TCO return ? */
	/* XXX: broken */
	if (0 && tree->head_type == RET
	    && tree->child_count
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
			printf("movq %s, %s\n", str, symstack((-2 - i) * 8));
			free_temp_reg(str);
		}

		printf("jmp _tco_%s\n", current_proc);
		return NULL;
	}

	/* return */
	if (tree->head_type == RET) {
		new_temp_reg();
		new_temp_mem();
		/* 
		 * Put the return expression's value
		 * in EAX and jump to the end of the
		 * routine
		 */
		printf("# return value\n");
		/*
		 * big things (like structs): copy to buffer in core
		 * and return pointer
		 */
		if (tree->child_count && type2siz(tree_typeof(tree->child[0])) > 8) {
			/* procedure calls already give pointers */
			if (check_proc_call(tree->child[0]) || tree->child[0]->head_type == ARRAY) {
				sto = codegen(tree->child[0]);
				printf("movq %s, %%rax\n", sto);
			} else if (tree->child[0]->head_type == DEREF
				|| tree->child[0]->head_type == DEREF_STRUCT_MEMB) {
				sto = codegen(tree->child[0]->child[0]);
				printf("movq %s, %%rax\n", sto);
			} else {
				sto = codegen(tree->child[0]);	
				printf("leaq %s, %%rax\n", sto);
			}
			printf("subq $48, %%rsp\n");
			printf("pushq %%rax\n");
			printf("pushq %%rbx\n");
			printf("pushq %%rcx\n");
			printf("pushq %%rdx\n");
			printf("pushq %%rsi\n");
			printf("pushq %%rdi\n");
			printf("leaq __return_buffer, %%rdi	# argument 0 to ___mymemcpy\n");
			printf("movq %%rax, %%rsi	# argument 1 to ___mymemcpy\n");
			printf("movq $%d, %%rdx		# argument 2 to ___mymemcpy\n", type2siz(tree_typeof(tree->child[0])));
			printf("call ___mymemcpy\n");
			printf("popq %%rdi\n");
			printf("popq %%rsi\n");
			printf("popq %%rdx\n");
			printf("popq %%rcx\n");
			printf("popq %%rbx\n");
			printf("popq %%rax\n");
			printf("addq $48, %%rsp\n");
			printf("leaq __return_buffer, %%rax\n");
		} else {
			/* code the return expression (if there is one) */
			if (tree->child_count)
				sto = codegen(tree->child[0]);

			if (tree->child_count && strcmp(sto, "%rax")) {
				/* 
				 * XXX: this assumes an int return value
				 * (which is the default, anyway) 
				 */
				printf("movq %s, %%rax # ret\n", sto);
			}
		}
		printf("jmp _ret_%s\n", current_proc);
		return NULL;
	}

	/*
	 * `switch' statement
	 * Note that the jump table has already been
	 * built and injected into .rodata this point
	 */
	if (tree->head_type == SWITCH) {
		/* 
		 * Retrieve the special decorative node that
		 * says which jump table number this has been
		 * mapped to. (see function `create_jump_tables')
		 */
		sscanf(get_tok_str(*(tree->child[tree->child_count - 1]->tok)), "%d", &jumptab);
		/*
		 * then get rid of it
		 */
		--tree->child_count;

		/*
		 * make a label for deep-nested `break'-ing
		 */
		lab1 = intl_label++;
		break_labels[++break_count] = lab1;
		
		/*
		 * Code the main switch argument,
		 * i.e. the `u' in switch(u) { ... },
		 * and put the result in a register
		 */
		sto = registerize(codegen(tree->child[0]));

		/*
		 * Some checks to see whether it is
		 * within the range of existing labels
		 * If not jump to a `default' label, if any.
		 */
		sto2 = get_temp_reg();

		printf("movq $%d, %s\n", switch_maxlab[jumptab], sto2);
		printf("cmpq %s, %s\n", sto2, sto);
		printf("jg jt%d_def\n", jumptab);
		
		printf("movq $%d, %s\n", 0, sto2);
		printf("cmpq %s, %s\n", sto2, sto);
		printf("jl jt%d_def\n", jumptab);

		free_temp_reg(sto2);

		/*
		 * The main switch arg becomes an index 
		 * into the jump table. Multiply it by
		 * eight bytes because we're doing byte
		 * addresses and things
		 * are a 64-bit word apart.
		 */
		printf("imulq $8, %s\n", sto);

		/*
		 * Fetch the jump table pointer
		 * into a register
		 */
		sto2 = get_temp_reg();
		printf("movq $jt%d, %s\n", jumptab, sto2);

		/*
		 * add index*8 to the jumptable pointer
		 */
		printf("addq %s, %s\n", sto, sto2);

		/*
		 * goto jump_table_pointer[index];
		 */
		printf("jmp (%s)\n", sto2);

		free_temp_reg(sto);
		free_temp_reg(sto2);

		/*
		 * now we code the inside of the switch
		 * statement, which contains regular code
		 * and a few special `case' labels, and
		 * `break's and `default' also
		 */
		casenum = 0;
		defset = 0;
		for (i = 1; i < tree->child_count; ++i) {
			if (tree->child[i]->head_type == SWITCH_CASE) {
				printf("jt%d_l%d:\n",
					jumptab, casenum++);
			} else if (tree->child[i]->head_type == SWITCH_BREAK) {
				printf("jmp jt%d_end\n", jumptab);
			} else if (tree->child[i]->head_type == SWITCH_DEFAULT) {
				if (defset)
					compiler_fail("only one `default' per `switch' please",
							findtok(tree->child[i]), 0, 0);
				defset = 1;
				printf("jt%d_def:\n", jumptab);
			} else {
				/*
				 * Normal code -- statement,
				 * so clear temp registers
				 */
				new_temp_mem();
				new_temp_reg();
				(void)codegen(tree->child[i]);
			}
		}

		/*
		 * close break-scope
		 */
		printf("IL%d:\n", lab1);
		--break_count;

		if (!defset)
			printf("jt%d_def:\n", jumptab);

		printf("jt%d_end:\n", jumptab);
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
		sto2 = get_temp_reg_siz(8);
		/* 
		 * XXX: assumes codegen() always
		 * returns int, which atm it does,
		 * but perhaps shouldn't
		 */
		printf("movq $0, %s\n", sto2);
		printf("cmp %s, %s\n", sto, sto2);
		printf("movq $1, %s\n", sto2);
		free_temp_reg(sto);
		printf("je cc%d\n", my_ccid);
		printf("movq $0, %s\n", sto2);
		printf("cc%d:\n", my_ccid);
		return sto2;
	}

	/* && - and with short-circuit */
	if (tree->head_type == CC_AND) {
		my_ccid = ccid++;
		sto2 = get_temp_reg_siz(8);
		/* 
		 * XXX: assumes codegen() always
		 * returns int, which atm it does,
		 * but perhaps shouldn't
		 */
		printf("movq $0, %s\n", sto2);
		for (i = 0; i < tree->child_count; ++i) {
			sto = codegen(tree->child[i]);
			printf("cmpq %s, %s\n", sto, sto2);
			printf("je cc%d\n", my_ccid);
			free_temp_reg(sto);
		}
		printf("movq $1, %s\n", sto2);
		printf("cc%d:\n", my_ccid);
		return sto2;
	}

	/* || - or with short-circuit */
	if (tree->head_type == CC_OR) {
		my_ccid = ccid++;
		sto2 = get_temp_reg_siz(8);
		/* 
		 * XXX: assumes codegen() always
		 * returns int, which atm it does,
		 * but perhaps shouldn't
		 */
		printf("movq $0, %s\n", sto2);
		for (i = 0; i < tree->child_count; ++i) {
			sto = codegen(tree->child[i]);
			printf("cmpq %s, %s\n", sto, sto2);
			printf("jne cc%d\n", my_ccid);
			free_temp_reg(sto);
		}
		printf("movq $0, %s\n", sto2);
		printf("jmp cc%d_2\n", my_ccid);
		printf("cc%d:\n", my_ccid);
		printf("movq $1, %s\n", sto2);
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

	/*
	 * Pre-increment, pre-decrement of array lvalue
	 * -- has been tested for array of char, int, char* 
	 */
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
		sto = get_temp_reg_siz(8);
		if (offs_siz != 1)
			printf("imulq $%d, %s\n", offs_siz, sto2);
		printf("addq %s, %s\n", sym_s, sto2);
		printf("movq %s, %s\n", sto2, sto);
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

		if (membsiz > 8) {
			/* 
			 * Rewrite large assignments as:
			 *
			 * (PROC_CALL:___mymemcpy 
			 *     (VARIABLE:b)
			 *     (ADDR (VARIABLE:a))
			 *     (SIZEOF (VARIABLE:a)))
			 *
			 * Where ___mymemcpy is a custom routine
			 * autoincluded in the compiled output.
			 * This should deal with struct assignments.
			 */

			faketok.type = TOK_IDENT;
			buf = newstr("___mymemcpy");
			faketok.start = buf;
			faketok.len = strlen(buf);
			faketok.from_line = 0;
			faketok.from_line = 1;

			fake_tree = new_exp_tree(PROC_CALL_MEMCPY, &faketok);
			if (tree->child[1]->head_type == DEREF
				|| tree->child[1]->head_type == DEREF_STRUCT_MEMB) {
				fake_tree_3 = *(tree->child[1]->child[0]);
			} else if (check_proc_call(tree->child[1]) || tree->child[1]->head_type == ARRAY) {
				fake_tree_3 = *(tree->child[1]);
			} else {
				fake_tree_3 = new_exp_tree(ADDR, NULL);
				add_child(&fake_tree_3, tree->child[1]);
			}
			fake_tree_4 = new_exp_tree(SIZEOF, NULL);
			add_child(&fake_tree_4, tree->child[0]);
			add_child(&fake_tree, tree->child[0]->child[0]);
			add_child(&fake_tree, alloc_exptree(fake_tree_3));
			add_child(&fake_tree, alloc_exptree(fake_tree_4));
			return codegen(alloc_exptree(fake_tree));
		}

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
	if (tree->head_type == ASGN && tree->child_count == 2
		&& tree->child[0]->head_type == VARIABLE) {

		if (type2siz(tree_typeof(tree->child[0])) > 8) {
			/* 
			 * Rewrite large assignments as:
			 *
			 * (PROC_CALL:___mymemcpy 
			 *     (ADDR (VARIABLE:b))
			 *     (ADDR (VARIABLE:a))
			 *     (SIZEOF (VARIABLE:a)))
			 *
			 * Where ___mymemcpy is a custom routine
			 * autoincluded in the compiled output.
			 * This should deal with struct assignments.
			 */

			faketok.type = TOK_IDENT;
			buf = newstr("___mymemcpy");
			faketok.start = buf;
			faketok.len = strlen(buf);
			faketok.from_line = 0;
			faketok.from_line = 1;

			fake_tree = new_exp_tree(PROC_CALL_MEMCPY, &faketok);
			fake_tree_2 = new_exp_tree(ADDR, NULL);
			add_child(&fake_tree_2, tree->child[0]);
			if (tree->child[1]->head_type == DEREF
				|| tree->child[1]->head_type == DEREF_STRUCT_MEMB) {
				fake_tree_3 = *(tree->child[1]->child[0]);
			} else if (check_proc_call(tree->child[1]) || tree->child[1]->head_type == ARRAY) {
				fake_tree_3 = *(tree->child[1]);
			} else {
				fake_tree_3 = new_exp_tree(ADDR, NULL);
				add_child(&fake_tree_3, tree->child[1]);
			}
			fake_tree_4 = new_exp_tree(SIZEOF, NULL);
			add_child(&fake_tree_4, tree->child[0]);
			add_child(&fake_tree, alloc_exptree(fake_tree_2));
			add_child(&fake_tree, alloc_exptree(fake_tree_3));
			add_child(&fake_tree, alloc_exptree(fake_tree_4));
			return codegen(alloc_exptree(fake_tree));
		}

		sym_s = sym_lookup(tree->child[0]->tok);

		/* 
		 * XXX: might not handle some complicated cases
		 * properly yet
		 * Note: there is no optimized code for variables
		 * because there are special rules for array variables,
		 * so might as well let the general case cover it all
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
		} else if (type2siz(tree_typeof(tree->child[0])) == 8) {
			/* 
			 * general case for 8-byte destination
			 */
			membsiz = type2siz(tree_typeof(tree->child[1]));
			/*
			 * Assigning an array ? well it gets
			 * implicitly converted to a pointer,
			 * so its size should be considered
			 * 8 bytes.
			 */
			if (tree_typeof(tree->child[1]).arr)
				membsiz = 8;
			/* n.b. codegen() converts stuff to int */
			sto = registerize_siz(codegen(tree->child[1]), membsiz);
			compiler_debug("simple variable assignment -- "
				       " looking for conversion suffix",
					 findtok(tree), 0, 0);
			sto2 = registerize_from(sto, membsiz);
			printf("movq %s, %s\n", sto2, sym_s);
			free_temp_reg(sto2);
			return sym_s;
		} else if (type2siz(tree_typeof(tree->child[0])) == 1) {
			/* 
			 * general case for 1-byte destination
			 */
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

		/* member size */
		membsiz = type2siz(
			deref_typeof(tree_typeof(tree->child[0]->child[0])));
		offs_siz = type2offs(
			deref_typeof(tree_typeof(tree->child[0]->child[0])));

		if (membsiz > 8) {
			/* 
			 * Rewrite large assignments as:
			 *
			 * (PROC_CALL:___mymemcpy 
			 *     (VARIABLE:b)
			 *     (ADDR (VARIABLE:a))
			 *     (SIZEOF (VARIABLE:a)))
			 *
			 * Where ___mymemcpy is a custom routine
			 * autoincluded in the compiled output.
			 * This should deal with struct assignments.
			 */

			faketok.type = TOK_IDENT;
			buf = newstr("___mymemcpy");
			faketok.start = buf;
			faketok.len = strlen(buf);
			faketok.from_line = 0;
			faketok.from_line = 1;

			fake_tree = new_exp_tree(PROC_CALL_MEMCPY, &faketok);
			if (tree->child[1]->head_type == DEREF
				|| tree->child[1]->head_type == DEREF_STRUCT_MEMB) {
				fake_tree_3 = *(tree->child[1]->child[0]);
			} else if (check_proc_call(tree->child[1]) || tree->child[1]->head_type == ARRAY) {
				fake_tree_3 = *(tree->child[1]);
			} else {
				fake_tree_3 = new_exp_tree(ADDR, NULL);
				add_child(&fake_tree_3, tree->child[1]);
			}

			fake_tree_2 = new_exp_tree(ADDR, NULL);
			add_child(&fake_tree_2, tree->child[0]);

			fake_tree_4 = new_exp_tree(SIZEOF, NULL);
			add_child(&fake_tree_4, tree->child[0]);
			add_child(&fake_tree, alloc_exptree(fake_tree_2));
			add_child(&fake_tree, alloc_exptree(fake_tree_3));
			add_child(&fake_tree, alloc_exptree(fake_tree_4));

			return codegen(alloc_exptree(fake_tree));
		}

		/* get base ptr */
		sym_s = codegen(tree->child[0]->child[0]);

		/* index expression */
		str = codegen(tree->child[0]->child[1]);
		sto2 = registerize_siz(str, membsiz);

		if (offs_siz != 1)
			printf("imulq $%d, %s\n", offs_siz, sto2);
		printf("addq %s, %s\n", sym_s, sto2);
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
	 * 
	 * XXX: TODO: this doesn't work for arrays of struct
	 * yet, probably a trivial fix though.
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
			printf("imulq $%d, %s\n", offs_siz, sto2);
		printf("addq %s, %s\n", sym_s, sto2);
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

		/*
		 * large things -> pointers
		 * convention
		 */
		if (membsiz > 8)
			do_deref = 0;

		if (do_deref)
			printf("%s (%s), %s\n", 
				move_conv_to_long(membsiz), sto2, sto);
		else
			printf("movq %s, %s\n", 
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
		fake_tree_2 = new_exp_tree(tree->head_type == INC ? 
			ADD : SUB, NULL);
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
		fake_tree_2 = new_exp_tree(tree->head_type == POST_INC ?
			ADD : SUB, NULL);
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
		sto = get_temp_reg_siz(8);
		printf("movq $%s, %s\n", 
			get_tok_str(*(tree->tok)),
			sto);
		return sto;
	}

	/* sizeof */
	/* XXX: might want this to be pre-inlined to integer nodes */
	if (tree->head_type == SIZEOF) {
		sto = get_temp_reg_siz(8);

		/* more hacks */
		if (type2offs(tree_typeof(tree->child[0])) == 0)
			printf("movq $%d, %s\n", 
				type2offs(tree_typeof(tree)),
				sto);
		else
			printf("movq $%d, %s\n", 
				type2offs(tree_typeof(tree->child[0])),
				sto);
		return sto;
	}

	/* 
	 * variable retrieval
	 * (array variables get handled earlier as a special case)
	 * -- converts char to int
	 */
	if (tree->head_type == VARIABLE) {
		sto = get_temp_reg_siz(8);
		sym_s = sym_lookup(tree->tok);
		membsiz = type2siz(sym_lookup_type(tree->tok));
		compiler_debug("variable retrieval -- trying conversion",
			findtok(tree), 0, 0);

		/*
		 * Special case for big-ass structs.
		 * They can't be fitted into a single
		 * word register, so just return their
		 * stack address (or core symbol for
		 * globals)
		 */
		if (membsiz > 8)
			return sym_lookup(tree->tok);

		printf("%s %s, %s\n",
			move_conv_to_long(membsiz),
			sym_s, sto);
		return sto;
	}

	/* arithmetic */
	if (tree->head_type == SUB_MASKED || (arith = arith_op(tree->head_type))
		&& tree->child_count) {

		/*
		 * A difference of two pointers is a special case:
		 * then, as far as I can tell, you can do:
		 * (pointerA - pointerB) / sizeof(*pointerA)
		 * to get the correct value, which adjusts for the
		 * multiplication effect by dividing.
		 */
		if (tree->head_type == SUB
			&& tree->child_count == 2
			&& (!tree_typeof(tree->child[0]).arr && tree_typeof(tree->child[0]).ptr)
			&& (!tree_typeof(tree->child[1]).arr && tree_typeof(tree->child[1]).ptr)) {

			exp_tree_t *special = alloc_exptree(new_exp_tree(DIV, tree->tok));
			tree->head_type = SUB_MASKED;
			add_child(special, tree);
			exp_tree_t *szeof = alloc_exptree(new_exp_tree(SIZEOF, tree->tok));
			exp_tree_t *deref = alloc_exptree(new_exp_tree(DEREF, tree->tok));
			add_child(deref, tree->child[0]);
			add_child(szeof, deref);
			add_child(special, szeof);

			return codegen(special);
		}

		if (tree->head_type == SUB_MASKED) {
			arith = arith_op(SUB);
			tree->head_type = SUB;
		}

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

		/* 
		 * If the pointer type is char *, then the mulitplier,
		 * obj_siz = sizeof(char) -- make sure to deref
		 * the type before asking for its size !
		 */
		if (ptr_arith_mode) 
			obj_siz = type2offs(deref_typeof(tree_typeof(tree)));

		/* 
		 * Find out which of the operands is the pointer,
		 * so that it won't get multiplied. if there is
		 * more than one pointer operand, complain and fail.
		 */
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
			oper = i ? arith : "movq";
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
					printf("imulq $%d, %s\n", obj_siz, str);
				printf("%s %s, %s\n", oper, str, sto);
				free_temp_reg(str);
			} else if (tree->child[i]->head_type == NUMBER) {
				str = registerize(sto);
				printf("%s $%s, %s\n", 
					oper, get_tok_str(*
						(tree->child[i]->tok)), str);
				if (stackstor) {
					printf("movq %s, %s\n", str, sto);
					free_temp_reg(str);
				}
			} else {
				/* general case */
				str = registerize(codegen(tree->child[i]));
				str2 = registerize(sto);
				printf("%s %s, %s\n", oper, str, str2);
				if (stackstor) {
					printf("movq %s, %s\n", str2, sto);
					free_temp_reg(str2);
				}
				free_temp_reg(str);
			}
		}
		if (stackstor)
			free_temp_mem(sto);
		return registerize(sto);
	}


	/* 
	 * division. division is special on x86.
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
			printf("movq %%rax, %s\n", sav1);
		}
		if (ts_used[3]) {	/* EDX in use ? */
			sav2 = get_temp_mem();
			printf("movq %%rdx, %s\n", sav2);
		}
		ts_used[0] = ts_used[3] = 1;
		/* code the dividend */
		str = codegen(tree->child[0]);
		/* put the 32-bit dividend in EAX */
		printf("movq %s, %%rax\n", str);
		free_temp_reg(str);
		free_temp_mem(str);
		/* clear EDX (higher 32 bits of 64-bit dividend) */
		printf("xor %%rdx, %%rdx\n");
		/* extend EAX sign to EDX */
		if (tree->head_type != MOD)
			printf("cdq\n");
		for (i = 1; i < tree->child_count; i++) {
			/* (can't idivl directly by a number, eh ?) */
			if(tree->child[i]->head_type == VARIABLE) {
				sym_s = sym_lookup(tree->child[i]->tok);
				printf("idivq %s\n", sym_s);
				if (tree->head_type != MOD) {
					printf("xor %%rdx, %%rdx\n");
					printf("cdq\n");
				}
			} else {
				str = codegen(tree->child[i]);
				printf("idivq %s\n", str);
				if (tree->head_type != MOD) {
					printf("xor %%rdx, %%rdx\n");
					printf("cdq\n");
				}
				free_temp_reg(str);
				free_temp_mem(str);
			}
		}
		/*
		 * move result (in EAX or EDX depending on operation)
		 * to some temporary storage 
		 */
		printf("movq %s, %s\n", 
			tree->head_type == MOD ? "%rdx" : "%rax",
			sto);
		/* restore or free EAX and EDX */
		if (sav1)
			printf("movq %s, %%rax\n", sav1);
		else
			free_temp_reg("%rax");
		if (sav2)
			printf("movq %s, %%rdx\n", sav2);
		else
			free_temp_reg("%rdx");
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
		int else_ret;	/* a flag raised when the "else return"
				* pattern is detected and can be optimized */
		lab1 = intl_label++;
		lab2 = intl_label++;
		/* codegen the conditional */
		sto = codegen(tree->child[0]);
		/* branch if the conditional is false */
		str = registerize(sto);
		str2 = get_temp_reg_siz(8);
		printf("movq $0, %s\n", str2);
		printf("cmpq %s, %s\n", str, str2);
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
		str2 = get_temp_reg_siz(8);
		printf("movq $0, %s\n", str2);
		printf("cmpq %s, %s\n", str, str2);
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

	/* do-while */
	if (tree->head_type == DOWHILE) {
		lab1 = intl_label++;
		lab2 = intl_label++;
		printf("IL%d: \n", lab1);
		/* open break-scope */
		break_labels[++break_count] = lab2;
		/* codegen the block */
		codegen(tree->child[1]);
		/* codegen the conditional */
		sto = codegen(tree->child[0]);
		/* branch back up the conditional is true */
		str = registerize(sto);
		str2 = get_temp_reg_siz(8);
		printf("movq $0, %s\n", str2);
		printf("cmpq %s, %s\n", str, str2);
		free_temp_reg(sto);
		free_temp_reg(str);
		free_temp_reg(str2);
		printf("jne IL%d\n", lab1);
		/* break label */
		printf("IL%d:\n", lab2);
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

	/* continue label */
	if (tree->head_type == CONTLAB) {
		printf("CL%d:\n", break_labels[break_count]);
		return NULL;
	}

	/* continue */
	if (tree->head_type == CONTINUE) {
		if (!break_count)
			codegen_fail("continue outside of a loop", tree->tok);
		printf("jmp CL%d # continue\n", break_labels[break_count]);
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
		sto2 = get_temp_reg_siz(8);
		str = codegen(tree->child[0]);
		printf("movq $0, %s\n", sto2);
		printf("subq %s, %s\n", str, sto2);
		printf("movq %s, %s\n", sto2, sto);
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
	/*
	if (findtok(tree)) {
		compiler_warn("I don't yet know how to code this", 
			findtok(tree), 0, 0);
		fprintf(stderr, "\n\n");
	}
	*/

	fprintf(stderr, "codegen_amd64: incapable of coding tree:\n");
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
	str = registerize_freemem(codegen(tree->child[0]));
	str2 = registerize_freemem(codegen(tree->child[1]));
	printf("cmpq %s, %s\n", str2, str);
	free_temp_reg(str);
	free_temp_reg(str2);
	sto3 = get_temp_reg_siz(8);	/* result */
	printf("movq $0, %s\n", sto3);
	printf("%s IL%d\n", oppcheck, intl_label);
	printf("movq $1, %s\n", sto3);
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
	int else_ret;		/* a flag raised when the "else return"
				 * pattern is detected and can be optimized */
	char *str, *str2, *sto, *sto2;
	int lab1 = intl_label++;
	int lab2 = intl_label++;
	/* optimized conditional */
	str = codegen(tree->child[0]->child[0]);
	str2 = codegen(tree->child[0]->child[1]);
	sto = registerize(str);
	sto2 = registerize(str2);
	printf("cmpq %s, %s\n", sto2, sto);
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
	printf("cmpq %s, %s\n", sto2, sto);
	free_temp_reg(sto);
	free_temp_reg(sto2);
	printf("%s IL%d\n", oppcheck, lab2);
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


