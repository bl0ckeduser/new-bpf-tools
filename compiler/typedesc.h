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
 */
typedef struct {
	int ty;			/* e.g. INT_DECL for "int" */
	int ptr;		/* e.g. 2 for "int **" */
	int arr;		/* e.g. 3 for "int [12][34][56]" */
	int *arr_dim;	/* e.g. {12, 34, 56} for "int [12][34][56]" */
} typedesc_t;

extern typedesc_t tree_typeof(exp_tree_t *tp);
extern void dump_td(typedesc_t);
extern typedesc_t deref_typeof(typedesc_t tp);

#endif
