#ifndef TYPEDESC_H
#define TYPEDESC_H

/* XXX: need more stuff for structs, also
 * maybe this should be designed in a more
 * structured recursive way -- "The Development 
 * of the C Language"
 * (http://www.cs.bell-labs.com/who/dmr/chist.html)
 * says: 
 * 
 * "given an object of any type, it should
 *  be possible to describe a new object that
 *  gathers several into an array, yields it from
 *  a function, or is a pointer to it"
 * 
 * well as it is here it's partly recursive like that.
 * unfortunately there is also some "clump a bunch
 * of stuff together and remember some precedence
 * rules"
 */

/* 
 * Arranged in order of precedence
 * (e.g. if there's an arr and a ptr, it's
 * an array of pointers, not the reverse)
 */
typedef struct {
	int arr;		/* e.g. 3 for "int [12][34][56]" */
	int *arr_dim;	/* e.g. {12, 34, 56} for "int [12][34][56]" */

	int ptr;		/* e.g. 2 for "int **" */

	int is_struct;
	struct struct_desc_s *struct_desc;

	int is_struct_name_ref;
	char* struct_name_ref;

	int ty;			/* e.g. INT_DECL for "int" */

} typedesc_t;

/*
 * this is how the compiler
 * thinks about structs.
 * and ...
 * it's a struct !
 * we've almost made it to scheme land,
 * get your wizard hats and 
 * "grand recursive order" badges ready
 */
typedef struct struct_desc_s {
	int cc;					/* number of tags */
	char snam[128];			/* structure name */
	char *name[128];		/* tag names */
	typedesc_t* typ[128];	/* tag types */
	int offs[128];			/* tag byte offsets */
	int bytes;				/* byte size of the whole thing */
} struct_desc_t;

extern typedesc_t tree_typeof(exp_tree_t *tp);
extern void dump_td(typedesc_t);
extern typedesc_t deref_typeof(typedesc_t tp);

#endif
