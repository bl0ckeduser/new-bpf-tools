#include "new_clown_vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO: stack for procedures code */

void clownVM_load(char **code_text, int code_toks)
{
	int i, j, k;
	int adr = 0;
	unsigned int *code;
	extern int numstr(char *);
	extern void fail(char *);
	extern void clownVM_run(unsigned int* program);

	if (!(code = malloc((code_toks + 1) * sizeof(int))))
		fail("malloc bytecode");

	for (i = 0; i < code_toks; i++) {
		if (code_text[i][strlen(code_text[i]) - 1] == '\n')
			code_text[i][strlen(code_text[i]) - 1] = 0;

		if (numstr(code_text[i])) {
			code[adr++] = atoi(code_text[i]);
		} else {
			for (j = 0; j < CLOWN_OPCODE_COUNT; ++j)
				if (!strcmp(clown_opcodes[j].mne, code_text[i]))
					code[adr++] = clown_opcodes[j].code;
		}
	}

	code[adr] = 255;

	clownVM_run(code);
	free(code);
}

void clownVM_run(unsigned int* program)
{
	unsigned int* storage;
	unsigned int* pc = program;
	int stor, stor2, stor3;
	int *adr = &stor, *adr2 = &stor2, op, ty, mode, *dat = &stor3, val;
	int a;
	extern void fail(char *);

	if (!(storage = calloc(2000000, sizeof(int))))
		fail("malloc clown vm storage");

	for(;; ++pc) {
		switch(*pc)
		{
			case op_do:
				adr = &storage[*++pc];
				op = *++pc;
				ty = *++pc;

				switch(ty) {
					case type_immediate:	*dat = *++pc;		break;
					case type_abstract:	*dat = storage[*++pc];	break;
				}

				switch(op) {
					case arith_assign:	*adr = *dat;	break;
					case arith_add:		*adr += *dat;	break;
					case arith_subtract:	*adr -= *dat;	break;
					case arith_multiply:	*adr *= *dat;	break;
					case arith_divide:	*adr /= *dat;	break;
				}

			break;

			case op_ptrto:
				storage[*++pc] = pc - program;
			break;

			case op_ptrfrom:
				pc = program + storage[*++pc];
			break;

			case op_zbptrto:
				adr = &storage[*++pc];
				val = *++pc;
				adr2 = &storage[*++pc];

				if(*adr > val)
					pc += *adr2;
			break;

			case op_echo:
				printf("%d\n", (int)storage[*++pc]);
			break;

			case op_end:
				goto cleanup;
			break;

			case op_adrset:
				adr = &storage[storage[*++pc]];
				adr2 = &storage[*++pc];
				*adr = *adr2;
			break;

			case op_adrget:
				adr = &storage[storage[*++pc]];
				adr2 = &storage[*++pc];
				*adr2 = *adr;
			break;

			default:
				printf("clown VM: illegal opcode %d\n", *pc);
				goto cleanup;
			break;
		}
	}

cleanup:
	free(storage);
}
