/*
 * Global initializer nasty cases - helper for x86 codegen
 */

#include <string.h>
#include <stdio.h>
#include "tree.h"
#include "typedesc.h"

extern char* get_tok_str(token_t t);
extern int type2offs(typedesc_t ty);

/* 
 * alexfru wrote:
 * (https://github.com/bl0ckeduser/new-bpf-tools/commit/546040096bb9a685adc2540f9e44d472c075012f#commitcomment-8233368)
 * 
 * "It's not hard to do.
 * 
 * Remember that every value (rvalue or lvalue) has a type and a way of being
 * computed (the mechanics; think of a sequence of assembly instructions
 * to get the value, unless it's a constant already). Basically, there are
 * two parallel pieces of information associated with every value. The type
 * describes how you can use the value (e.g. you can't dereference an integer
 * or shift a pointer), and the mechanics says how this value is obtained.
 * 
 * out[0] is of type char; it's calculated by dereferencing (the address
 * of the beginning of out + 0*sizeof(char))
 * 
 * out is of type array of 1 char; there's nothing to calculate here,
 * just use the address of the beginning of out.
 * 
 * &out is of type pointer to array of 1 char; there's nothing to calculate
 * here either (you just need to support taking addresses of arrays and
 * properly note the resultant type), use the same address as in the above.
 * 
 * When you initialize outPointer with either out or &out, you should
 * look at how the initializer (out or &out) is calculated. If behind
 * the calculation is just a numeric constant or a global address or a
 * sum/difference of a global address and a numeric constant, you've got
 * a constant initializer. The type information here plays a somewhat
 * secondary role. You should use it to perform type checks (in order
 * to reject invalid initializations like initializing a pointer with a
 * structure) and provide default conversions (if necessary). In many cases
 * (and the x86 platform is one such case) conversions between a pointer
 * and an integer or between pointers of/to different types amount to doing
 * no more than just changing the type of the value. They don't change the
 * value or the mechanics of obtaining it.
 * 
 * IOW, if you see that the initializer can be used as is (if it's
 * a number or an address) or require a simple adjustment that can be
 * done by relocation (as in the case of "address_of_variable_or_array +/-
 * integer"), the initializer is technically good. If the initializer's value
 * is not a constant or a simple global address and is instead a result
 * of multiplication, function call, memory read, size/bitvalue-changing
 * type cast and most other operations that can't be done at compile time
 * or with relocation, it can't be used as an initializer.
 * 
 * HTH"
 *
 *
 * I'll try to sort of follow that perhaps incompletely at first.
 * Anyway, can grow from here.
 */

/*
 * Gives GNU GAS x86 string for an initializer,
 * to be sticked into a '.long' directive
 */
int get_global_initializer_string_gnux86(exp_tree_t *t, char *str_dest)
{
	char buff[1024], buff2[1024];
	int offset;

	/*
	 * XXX: tree pattern matching would be very nice to have here,
	 * if i ever generalize that matching code i have i should stick
	 * it in here
	 */

	/*
	 * number: the number itself
	 */
	if (t->head_type == NUMBER) {
		strcpy(str_dest, get_tok_str(*(t->tok)));
		return 1;
	}

	/*
	 * &foo -> valid if foo is a global variable;
	 * then give simply 'foo'. Well let's assume it exists,
	 * if it doesn't the assembler will fuck up.
	 */
	if (t->head_type == ADDR
		&& t->child_count == 1
		&& t->child[0]->head_type == VARIABLE) {

		strcpy(str_dest, get_tok_str(*(t->child[0]->tok)));

		return 1;
	}

	/*
	 * &foo[n]
	 * note, n has been constfold'ed already by codegen_x86 at this point
	 */
	if (t->head_type == ADDR
		&& t->child_count == 1
		&& t->child[0]->head_type == ARRAY
		&& t->child[0]->child_count == 2
		&& t->child[0]->child[0]->head_type == VARIABLE
		&& t->child[0]->child[1]->head_type == NUMBER) {
		
		int membsiz = type2offs(deref_typeof(
			tree_typeof(t->child[0]->child[0])));

		if (membsiz == 0) {
			compiler_warn("I don't currently support forward type references", findtok(t), 0, 0);
			return 0;
		}

		buff[0] = 0;
		strcat(buff, get_tok_str(*(t->child[0]->child[0]->tok)));
		strcat(buff, " + ");
		sscanf(get_tok_str(*(t->child[0]->child[1]->tok)), "%d", &offset);
		sprintf(buff2, "%d", offset * membsiz);
		strcat(buff, buff2);
		strcpy(str_dest, buff);

		return 1;
	}

	return 0;
}

