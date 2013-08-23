/* Pretty compiler-failure error */

#include "tokenizer.h"
#include "tokens.h"
#include <string.h>

enum {
	CF_ERROR,
	CF_WARN,
	CF_DEBUG
};

/* internal prototype */
void compiler_fail_int(char *message, token_t *token,
	int in_line, int in_chr, int mode);

void compiler_fail(char *message, token_t *token,
	int in_line, int in_chr)
{
	compiler_fail_int(message, token, in_line, in_chr, CF_ERROR);
}

void compiler_warn(char *message, token_t *token,
	int in_line, int in_chr)
{
	compiler_fail_int(message, token, in_line, in_chr, CF_WARN);
}

void compiler_debug(char *message, token_t *token,
	int in_line, int in_chr)
{
#ifdef DEBUG
	compiler_fail_int(message, token, in_line, in_chr, CF_DEBUG);
#endif
}

void compiler_fail_int(char *message, token_t *token,
	int in_line, int in_chr, int mode)
{
	char buf[1024];
	int line;
	int chr;
	int i;

	/* check for specific line/chr override
	 * (used for special cases like end of line) */
	line = in_line ? in_line : token->from_line;
	chr = in_line ? in_chr : token->from_char;

	fflush(stdout);
	fflush(stderr);

	/* copy the originating code line to a buffer,
	 * then print this to stderr */
	strncpy(buf, code_lines[line], 1024);
	buf[1023] = 0;
	for (i = 0; code_lines[line]; ++i)
		if (buf[i] == '\n') {
			buf[i] = 0;
			break;
		}
	fprintf(stderr, "%s\n", buf);

	/* put a little arrow under the offending
	 * character */
	for (i = 1; i < chr; ++i)
		if (buf[i - 1] == '\t')
			fputc(buf[i - 1], stderr);
		else
			fputc(' ', stderr);
	fputc('^', stderr);
	fputc('\n', stderr);

	/* finally display line number and error
	 * description */
	if (mode == CF_DEBUG)
		fprintf(stderr, "debug: ");
	else if (mode == CF_ERROR)
		fprintf(stderr, "error: ");
	else
		fprintf(stderr, "warning: ");
	fprintf(stderr, "line %d: %s\n", 
		line,
		message);
	if (mode == CF_ERROR)
		exit(1);
}
