#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "tokenizer.h"

void compiler_fail_int(char *message, token_t *token,
	int in_line, int in_chr, int mode);

void compiler_fail(char *message, token_t *token,
	int in_line, int in_chr);

void compiler_warn(char *message, token_t *token,
	int in_line, int in_chr);

void compiler_debug(char *message, token_t *token,
	int in_line, int in_chr);

void compiler_fail_int(char *message, token_t *token,
	int in_line, int in_chr, int mode);

#endif
