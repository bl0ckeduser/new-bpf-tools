#ifndef GENERAL_H
#define GENERAL_H

#include "tree.h"

extern void fail(char* mesg);
extern void sanity_requires(int exp);
extern token_t *findtok(exp_tree_t *et);

#endif
