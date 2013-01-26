#ifndef CLOWN_VM_H
#define CLOWN_VM_H

extern void clownVM_load(char **code_text, int code_toks);

typedef struct {
	char* mne;
	int code;
} clown_opcode_t;

#define CLOWN_OPCODE_COUNT 8
static clown_opcode_t clown_opcodes[] = {
	{ "Do", 10 },
	{ "Echo", 11 },
	{ "PtrTo", 12},
	{ "PtrFrom", 13 },
	{ "zbPtrTo", 35 },
	{ "AdrSet", 94 },
	{ "AdrGet", 95 },
	{ "End",  255 },
};

#define op_do 10
#define op_echo 11
#define op_ptrto 12
#define op_ptrfrom 13
#define op_zbptrto 35
#define op_adrset 94
#define op_adrget 95
#define op_end 255

#define type_immediate 1
#define type_abstract 2

#define arith_assign 10
#define arith_add 20
#define arith_subtract 30
#define arith_multiply 40
#define arith_divide 50

#define jump_forward 10
#define jump_backward 20

#endif
