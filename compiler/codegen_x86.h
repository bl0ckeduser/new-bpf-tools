#ifndef CODEGEN_H
#define CODEGEN_H

extern char* codegen(exp_tree_t* tree);

/* General-purpose 32-bit x86 registers.
 * Did I forget any ? */
#define TEMP_REGISTERS 6
static char* temp_reg[TEMP_REGISTERS] = {
	"%eax",
	"%ebx",
	"%ecx",
	"%edx",
	"%esi",
	"%edi" 
};

#define TEMP_MEM 16
static char* temp_mem[TEMP_MEM];

#endif
