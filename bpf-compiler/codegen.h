#ifndef CODEGEN_H
#define CODEGEN_H

typedef struct codegen {
	int adr;
	int bytes;
} codegen_t;

extern codegen_t codegen(exp_tree_t* tree);

typedef struct {
	char* name;
	char* instr;
	int arg_count;
	int arg_type[32];
} bpf_instr_t;

#define INSTR_COUNT 8

static bpf_instr_t instr_info[] = {
	{ "echo", "Echo", 1, { 1 }},
	{ "draw", "vDraw", 8, { 1, 1, 0, 0, 0, 0, 0, 0 }},
	{ "wait", "Wait", 2, { 0, 0 }},
	{ "cmx", "cmx", 1, { 2 }},
	{ "cmy", "cmy", 1, { 2 }},
	{ "mx", "mx", 1, { 2 }},
	{ "my", "my", 1, { 2 }},
	{ "outputdraw", "OutputDraw", 0, { 0 }}
};

#endif
