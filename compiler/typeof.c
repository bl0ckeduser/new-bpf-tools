#include "tree.h"
#include "tokens.h"
#include "typedesc.h"
#include <string.h>
#include <stdio.h>

/*
 * This module tries to figure out the type of
 * an expression tree. It is meant to work
 * with the x86 code generator found in the
 * file codegen_x86.c
 */

char err_buf[1024];

/* Internal-use prototype */
typedesc_t tree_typeof_iter(typedesc_t, exp_tree_t*);

/* 
 * Hooks to x86 codegen symbol-type query 
 * / constant dispatch table stuff
 */
extern typedesc_t sym_lookup_type(token_t* tok);
extern int int_type_decl(char ty);
extern int decl2siz(int);
extern typedesc_t func_ret_typ(char *func_nam);
extern char* get_tok_str(token_t t);
extern void fail(char *);
extern struct_desc_t *find_named_struct_desc(char *s);

/* hook to error printout code */
void compiler_fail(char *message, token_t *token,
	int in_line, int in_chr);

/* hook to token finder */
extern token_t *findtok(exp_tree_t *et);

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
	td.is_struct = 0;
	td.is_struct_name_ref = 0;
	return td;
}

/* 
 * Type description => size of datum in bytes
 */
int type2siz(typedesc_t ty)
{
	/*
	 * Struct alone, and not a pointer to it
	 */
	if (ty.ptr == 0 && ty.is_struct) {
		return ty.struct_desc->bytes;
	}

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

int arr_dim_prod(typedesc_t ty)
{
	int prod, i;
	for (prod = 1, i = 0; i < ty.arr; ++i) {
		prod *= ty.arr_dim[i];
	}
	return prod;
}

int type2offs(typedesc_t ty)
{
	int prod, i;
	if (ty.arr >= 0 && ty.arr_dim) {
		prod = arr_dim_prod(ty);
		ty.arr = 0;
		return type2siz(ty) * prod;
	}
	return type2siz(ty);
}

/* get Nth array dimension from array declaration */
int get_arr_dim(exp_tree_t *decl, int n)
{
	int array = 0;
	int i;
	int r;
	for (i = 0; i < decl->child_count; ++i)
		if (decl->child[i]->head_type == ARRAY_DIM)
			if (array++ == n) {
				sscanf(get_tok_str(*(decl->child[i]->tok)), "%d", &r);
				return r;
			}
	return -1;
}

/* 
 * Count pointer-qualification stars
 * in a declaration syntax tree,
 * as in e.g. "int ***herp_derp"
 */
void count_stars(exp_tree_t *dc, int *stars)
{
	int j, newlen = 0;
	*stars = 0;
	for (j = 0; j < dc->child_count; ++j)
		if (dc->child[j]->head_type == DECL_STAR)
			++*stars;
}

/* 
 * Discard pointer-qualification stars
 * from a declaration syntax tree
 */
void discard_stars(exp_tree_t *dc)
{
	int j, newlen = 0, count = 0;
	for (j = 0; j < dc->child_count; ++j)
		if (dc->child[j]->head_type == DECL_STAR)
			newlen = newlen ? newlen : j,
			++count,
			dc->child[j]->head_type = NULL_TREE;
	if (count)
		dc->child_count = newlen;
}


/* 
 * Check if a declaration tree is for an array,
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
 * Find out dimensions and dimension size-numbers from
 * an array declaration parse tree
 */
void parse_array_info(typedesc_t *typedat, exp_tree_t *et)
{
	int i;
	typedat->arr = check_array(et);
	typedat->arr_dim = malloc(typedat->arr * sizeof(int));
	for (i = 0; i < typedat->arr; ++i)
		typedat->arr_dim[i] = get_arr_dim(et, i); 
}

/*
 * Parse a declarator syntax tree into
 * a type descriptor structure
 */
void parse_type(exp_tree_t *dc, typedesc_t *typedat,
				typedesc_t *array_base_type, int *objsiz, int decl,
				exp_tree_t *father)
{
	int stars = 0;
	int newlen = 0;
	int i, j, k;

	if (decl == STRUCT_DECL) {
		compiler_fail("sorry I can't do structs in this context yet",
					findtok(father), 0, 0);
	}

	/* count pointer stars */
	count_stars(dc, &stars);

	/* build type description for the object */
	*typedat = mk_typedesc(decl, stars, 0);
	parse_array_info(typedat, dc);
	*array_base_type = *typedat;
	array_base_type->arr = 0;

	if (decl == NAMED_STRUCT_DECL) {
		typedat->is_struct_name_ref = 1;
		typedat->struct_name_ref = malloc(128);
		strcpy(typedat->struct_name_ref, get_tok_str(*(father->tok)));
	}

	/* figure out size of the whole object in bytes */
	*objsiz = type2offs(*typedat);

	/* 
	 * Use 4 bytes instead of 1 for chars in char arrays.
	 * unfortunately, parts of the code still expect this. 
	 * XXX: find a better fix to this problem
	 */
	if (check_array(dc) && array_base_type->ptr == 0
		&& array_base_type->ty == CHAR_DECL) {
		*objsiz *= 4;
		array_base_type->ty = INT_DECL;
	}

	/* Add some padding; otherwise werid glitches happen */
	if (*objsiz < 4 && !check_array(dc))
		*objsiz = 4;
}

/*
 * Parse a struct type declarator into
 * a type description structure
 */
typedesc_t struct_tree_2_typedesc(exp_tree_t *tree, int *bytecount,
	struct_desc_t **sd_arg)
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
	int tag_offs;
	char tag_name[64];
	int struct_bytes;
	typedesc_t *heap_typ;
	int padding;
	int struct_pass;

	/* build the struct's type description
	 * (derived types like pointers come later in the
	 * declaration children) */
	struct_base.ty = 0;		
	struct_base.arr = struct_base.ptr = 0;

	/* allocate struct-description structure
	 * (yes this is starting to get meta) */
	*sd_arg = malloc(sizeof(struct_desc_t));

	/* track structure name */
	strcpy((*sd_arg)->snam, get_tok_str(*(tree->tok)));

	/* figure out tag count */
	(*sd_arg)->cc = 0;
	for (i = 0; i < tree->child_count; ++i)
		if (tree->child[i]->head_type == DECL_CHILD)
			break;
		else
			(*sd_arg)->cc += 1;

	/* iterate over tags */
	struct_pass = 0;
	tag_offs = 0;
struct_pass_iter:
	/*
	 * For whatever mysterious reason,
	 * compiled code appears to segfault on FreeBSD 32-bit
	 * unless I align every tag to 16 bytes
	 * and code all non-array tags first.
	 * (?????)
	 */
	for (i = 0; i < (*sd_arg)->cc; ++i) {
		dc = tree->child[i]->child[0];
		/* get tag name */
		strcpy(tag_name, get_tok_str(*(dc->child[0]->tok)));

		/* get information on the tag's type */
		/* XXX: tags can't be structs themselves yet ! */
		parse_type(dc, &tag_type, &array_base_type, &objsiz, 
			tree->child[i]->head_type, tree->child[i]);

		if (!struct_pass && check_array(dc))
			continue;
		if (struct_pass && !check_array(dc))
			continue;

		#ifdef DEBUG
			fprintf(stderr, "struct `%s' -> tag `%s' -> ", 
				get_tok_str(*(tree->tok)),
				tag_name);
			fprintf(stderr, "size: %d bytes, offset: %d\n", 
				objsiz, tag_offs);
			dump_td(tag_type);
		#endif

		/* mystery segfault-proofing padding */
		while (tag_offs % 16)
			++tag_offs;

		/* write tag data to struct description */
		(*sd_arg)->offs[i] = tag_offs;
		(*sd_arg)->name[i] = malloc(128);
		strcpy((*sd_arg)->name[i], tag_name);
		/* (gotta *copy* the tag type data to heap because it's stack 
		 * -- otherwise funny things happen when you try to 
		 * read it outside of this function) */
		heap_typ = malloc(sizeof(typedesc_t));
		memcpy(heap_typ, &tag_type, sizeof(typedesc_t));
		(*sd_arg)->typ[i] = heap_typ;

		/* bump tag offset calculation */
		tag_offs += objsiz;
	}

	if (!struct_pass++)
		goto struct_pass_iter;

	/* size in bytes of the whole struct */
	*bytecount = tag_offs;

	/* bind struct desc to struct base type desc */
	struct_base.struct_desc = (*sd_arg);
	struct_base.is_struct = 1;

	return struct_base;
}

/* offset (in bytes) of a tag in a structure */
int struct_tag_offs(typedesc_t stru, char *tag_name)
{
	int i;

	/* figure out the type of the tag and return it */
	for (i = 0; i < stru.struct_desc->cc; ++i) {
		if (!strcmp(stru.struct_desc->name[i], tag_name)) {
			return stru.struct_desc->offs[i];
		}
	}

	fail("could not find a struct tag offset");
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
 * Write out typdesc_t data
 */
void dump_td(typedesc_t td) {
	extern void dump_td_iter(typedesc_t td, int depth);
	dump_td_iter(td, 0);
}
void indent(FILE *out, int len) {
	while (len --> 0)
		fprintf(out, "   ");
}
void dump_td_iter(typedesc_t td, int depth)
{
	int i;
	indent(stderr, depth);
	fprintf(stderr, "==== dump_td ======\n");
	if (td.ptr) {
		indent(stderr, depth);
		fprintf(stderr, "ptr: %d\n", td.ptr);
	}
	if (td.arr) {
		indent(stderr, depth);
		fprintf(stderr, "arr: %d\n", td.arr);
		indent(stderr, depth);
		fprintf(stderr, "arr_dim: ");
		if (td.arr_dim) {
			indent(stderr, depth);
			for (i = 0; i < td.arr; ++i) 
				fprintf(stderr, "%d:%d ", i, td.arr_dim[i]);
			fprintf(stderr, "\n");
		}
	}
	if (td.ty == NAMED_STRUCT_DECL && td.is_struct_name_ref) {
		indent(stderr, depth);
		fprintf(stderr, "** named struct **\n");
		indent(stderr, depth);
		fprintf(stderr, "named struct ref: %s\n", td.struct_name_ref);
	}
	else if (td.is_struct) {
		indent(stderr, depth);
		fprintf(stderr, "** struct **\n");
		indent(stderr, depth);
		fprintf(stderr, "struct_desc ptr = %p\n", td.struct_desc);
		indent(stderr, depth);
		fprintf(stderr, "tags: %d\n", td.struct_desc->cc);
		for (i = 0; i < td.struct_desc->cc; ++i) {
			indent(stderr, depth);
			fprintf(stderr, "tag: %s\n", td.struct_desc->name[i]);
			dump_td_iter(*(td.struct_desc->typ[i]), depth + 1);
		}
	} else {
		indent(stderr, depth);
		fprintf(stderr, "ty: %s\n", tree_nam[td.ty]);
	}
	indent(stderr, depth);
	fprintf(stderr, "===================\n");
}

/*
 * Dereference type description
 * e.g. int * => int
 */
typedesc_t deref_typeof(typedesc_t td)
{
	if (td.arr) {
			--td.arr;
			if (td.arr_dim)
				++td.arr_dim;
	}
	else
		--td.ptr;
	return td;
}

/*
 * Check if a tree type is
 * an arithmetic operation
 */
int is_arith_op(char ty)
{
	return ty == ADD
		|| ty == MULT
		|| ty == SUB
		|| ty == DIV
		|| ty == MOD;
}

/*
 * Try to figure out the type of an expression tree
 */
typedesc_t tree_typeof(exp_tree_t *tp)
{
	typedesc_t td;

	/* create `unknown' default result */
	td.ty = TO_UNK;
	td.ptr = 0;
	td.arr = 0;
	td.arr_dim = NULL;
	td.is_struct = 0;
	td.is_struct_name_ref = 0;

	td = tree_typeof_iter(td, tp);

	if (td.ty == NAMED_STRUCT_DECL && td.is_struct_name_ref) {
		td.is_struct = 1;
		td.is_struct_name_ref = 0;
		td.ty = STRUCT_DECL;
		td.struct_desc = find_named_struct_desc(td.struct_name_ref);
	}

	return td;
}

/*
 * The actual recursive loop part for trying to figure
 * out the type of an expression tree
 */
typedesc_t tree_typeof_iter(typedesc_t td, exp_tree_t* tp)
{
	int i;
	int max_decl, max_siz, max_ptr, siz;
	typedesc_t ctd;
	typedesc_t struct_typ, tag_typ;
	char tag_name[128];
	struct_desc_t *sd;
	int bc;

	/* 
	 * If a block has one child, find the type of that,
	 * otherwise fail.
	 */
	if (tp->head_type == BLOCK && tp->child_count) {
		if (tp->child_count == 1) {
			return tree_typeof_iter(td, tp->child[0]);
		} else {
			td.ty = TO_UNK;
			return td;
		}
	}

	if (tp->head_type == STRUCT_MEMB || tp->head_type == DEREF_STRUCT_MEMB) {
		/* find type of base */
		struct_typ = tree_typeof(tp->child[0]);
		strcpy(tag_name, get_tok_str(*(tp->child[1]->tok)));

		if (tp->head_type == DEREF_STRUCT_MEMB)
			struct_typ = deref_typeof(struct_typ);

		if (struct_typ.arr || struct_typ.ptr)
			compiler_fail("you can't apply the `.' operator on a"
						 " non-structure...",
				findtok(tp->child[0]), 0, 0);

		#if 0
		#ifdef DEBUG
			fprintf(stderr, "******************************************\n");
			fprintf(stderr, "trying to figure out the type of a `.' expr\n");
			fprintf(stderr, "---------------- base struct type: \n");
			dump_td_iter(struct_typ, 1);
			fprintf(stderr, "---------------------------------- \n");
			fprintf(stderr, "tag name: %s\n", tag_name);
			fprintf(stderr, "******************************************\n");
		#endif
		#endif
		
		/* figure out the type of the tag and return it */
		for (i = 0; i < struct_typ.struct_desc->cc; ++i) {
			if (!strcmp(struct_typ.struct_desc->name[i], tag_name)) {
				return *(struct_typ.struct_desc->typ[i]);
			}
		}

		/* the tag doesn't even exist, come on !!! */
		sprintf(err_buf, "a structure of type `%s' has no tag `%s'",
			struct_typ.struct_desc->snam,
			tag_name);
		compiler_fail(err_buf, findtok(tp->child[0]), 0, 0);
	}

	/*
	 * Explicit casts are what this module is really
	 * meant to parse and they give their type away ;)
	 *
	 * note: argument declarations use the same parse as casts for now
	 * 	and also function return types
	 */
	if ((tp->head_type == CAST || tp->head_type == ARG
		|| tp->head_type == PROC) 
			&& tp->child[0]->head_type == CAST_TYPE) {
		if (tp->child[0]->child[0]->head_type != BASE_TYPE
			|| tp->child[0]->child[0]->child_count != 1) {
			/* XXX: TODO: casts to non-basic-type-based stuff */
			td.ty = TO_UNK;
			return td;
		} else {
			/*
			 * e.g. "(int *)3" parses to
		 	 * (CAST 
			 *   (CAST_TYPE 
			 *     (BASE_TYPE
			 *       (INT_DECL)) 
			 * 	   (DECL_STAR))
			 *   (NUMBER:3))
			 */
			td.ty = tp->child[0]->child[0]->child[0]->head_type;
			td.arr = 0;
			td.ptr = tp->child[0]->child_count - 1;
			td.is_struct = 0;
			td.is_struct_name_ref = 0;
			if (td.ty == STRUCT_DECL) {
				struct_tree_2_typedesc(tp->child[0]->child[0]->child[0], &bc,
					&sd);
				td.is_struct = 1;
				td.struct_desc = sd;
			}
			if (td.ty == NAMED_STRUCT_DECL) {
				td.is_struct = 1;
				td.is_struct_name_ref = 0;
				td.ty = STRUCT_DECL;
				td.struct_desc = find_named_struct_desc(
					get_tok_str(*(tp->child[0]->child[0]->child[0]->tok)));
				return td;
			}
			return td;
		}
	}

	/*
	 * If it's a plain variable, query
	 * the codegen's symbol-type table.
	 *
	 * XXX: as it is, codegen() on a
	 * VARIABLE tree of any type converts
	 * it to int,
	 * (or to a pointer type for arrays
	 * and pointers, but have same size as int)
	 * so this might be
	 * inconsistent / confusing
	 */
	if (tp->head_type == VARIABLE) {
		return sym_lookup_type(tp->tok);
	}

	/*
	 * Integers constants are given
	 * type `int', not `char'
	 *
	 * XXX: if the language gets
	 * bigger this should be changed
	 * to support e.g. long constants
	 */
	if (tp->head_type == NUMBER) {
		td.ty = INT_DECL;
		return td;
	}

	/*
	 * typeof(&u).ptr = typeof(u).ptr + 1
	 */
	if (tp->head_type == ADDR) {
		td = tree_typeof_iter(td, tp->child[0]);
		++td.ptr;
		return td;
	}

	/*
	 * typeof(*u).ptr = typeof(u).ptr - 1
	 */
	if (tp->head_type == DEREF) {
		td = tree_typeof_iter(td, tp->child[0]);
		td = deref_typeof(td);
		return td;
	}

	/*
	 * Array access
	 */
	if (tp->head_type == ARRAY || tp->head_type == ARRAY_ADR) {
		/* type of base */
		td = tree_typeof_iter(td, tp->child[0]);
		/* dereference by number of []  - 1*/
		for (i = 0; i < tp->child_count - 1; ++i)
			td = deref_typeof(td);
		return td;
	}

	/*
	 * Procedure call return values:
	 * query the codegen's return types table.
	 */
	if (tp->head_type == PROC_CALL) {
		return func_ret_typ(get_tok_str(*(tp->tok)));
	}

	/* 
	 * For assignments, use the type of
	 * the lvalue. (Note that compound
	 * assignments do not have their
	 * own tree-head-types and get
	 * fully converted to non-compound
	 * syntax).
	 */
	if (tp->head_type == ASGN)
		return tree_typeof_iter(td, tp->child[0]);

	/*
	 * For arithmetic, promote to the
	 * pointer operand's type,
	 * if the operation is an addition
	 * or subtraction (pointer arithmetic
	 * applies).
	 * 
	 *		int *i;
	 *		i + 1 + 2; <-- type "int *"
	 *
	 *		int i;
	 * 		3 + 1 + &x;	<-- type "int *"
	 *
	 * (see K&R 2nd edition (ANSI C89), page 205, A7.7)
	 * 
	 * If two pointers are being subtracted
	 * return type "int".
	 * XXX: Actually it should be "ptrdiff_t"
	 * which is defined in <stddef.h>
	 * (see K&R 2nd edition (ANSI C89), page 206, A7.7)
	 *
	 * If pointers are being multiplied or
	 * divided or remainder'ed, put up
	 * a warning message.
	 *
	 * Operations involving char see their
	 * result converted to int, because
	 * codegen() is always supposed to give
	 * an int-typed result the way the code
	 * generator is currently designed.
	 */
	if (is_arith_op(tp->head_type)) {
		/* initialize type with data from first member */
		max_siz = decl2siz((ctd = tree_typeof(tp->child[0])).ty);
		max_decl = ctd.ty;
		max_ptr = ctd.arr + ctd.ptr;

		/* promote to pointer type if any */
		for (i = 0; i < tp->child_count; ++i) {
			if ((siz = decl2siz((ctd = tree_typeof(tp->child[i])).ty))
				&& siz >= max_siz
				&& (ctd.arr + ctd.ptr) > max_ptr) {
				max_siz = siz;
				max_decl = ctd.ty;
				max_ptr = ctd.arr + ctd.ptr;
			}
		}

		/* populate typedesc field */
		td.ty = max_decl;
		td.arr = 0;
		td.ptr = max_ptr;

		/* handle pointer subtraction case */
		if (tp->head_type == SUB && td.ptr && tp->child_count == 2
			&& (tree_typeof(tp->child[0]).ptr || tree_typeof(tp->child[0]).arr)
			&& (tree_typeof(tp->child[1]).ptr || tree_typeof(tp->child[1]).arr)) {
			td.ty = INT_DECL;	/* XXX: should be "ptrdiff_t" according
								 * to K&R 2 */
			td.arr = td.ptr = 0;
		}

		/* promote char to int */
		if (td.ty == CHAR_DECL && td.arr == 0 && td.ptr == 0)
			td.ty = INT_DECL;

		/* warning on strange arithmetic */
		if (!(tp->head_type == ADD || tp->head_type == SUB) && td.ptr)
			compiler_fail("please don't use pointers in incomprehensible ways",
				findtok(tp), 0, 0);

		return td;
	}

	/*
	 * String constants have type "char *"
	 */
	if (tp->head_type == STR_CONST) {
		td.ty = CHAR_DECL;
		td.ptr = 1;
		td.arr = 0;
	}

	/*
	 * For relationals and logical ops, return int
	 */
	if (tp->head_type == LT
		 || tp->head_type == LTE
		 || tp->head_type == GT
		 || tp->head_type == GTE
		 || tp->head_type == EQL
		 || tp->head_type == NEQL
 		 || tp->head_type == CC_OR
 		 || tp->head_type == CC_AND
		 || tp->head_type == CC_NOT) {
		td.ty = INT_DECL;
		td.ptr = td.arr = 0;
		return td;
	}

	/*
	 * Increments, decrements, negative sign:
	 * return type of operand
	 */
	if (tp->head_type == INC
		 || tp->head_type == DEC
		 || tp->head_type == POST_INC
		 || tp->head_type == POST_DEC
		 || tp->head_type == NEGATIVE) {
		return tree_typeof_iter(td, tp->child[0]);
	}

	/* 
	 * If all else fails, just give the original thing
	 */
	return td;
}
