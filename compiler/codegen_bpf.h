#ifndef CODEGEN_H
#define CODEGEN_H

typedef struct codegen {
	int adr;
	int bytes;
} codegen_t;

codegen_t codegen(exp_tree_t* tree);

typedef struct {
	char* name;
	char* instr;
	int arg_count;
	int arg_type[32];
} bpf_instr_t;

#define INSTR_COUNT 8

extern bpf_instr_t instr_info[];

#endif
