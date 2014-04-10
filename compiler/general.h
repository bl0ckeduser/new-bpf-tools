#ifndef GENERAL_H
#define GENERAL_H

#include "tree.h"

void fail(char* mesg);
void sanity_requires(int exp);
token_t *findtok(exp_tree_t *et);

#endif
