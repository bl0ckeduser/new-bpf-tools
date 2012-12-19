#ifndef CODEGEN_H
#define CODEGEN_H

typedef struct codegen {
	int adr;
	int bytes;
} codegen_t;

extern codegen_t codegen(exp_tree_t* tree);

#endif
