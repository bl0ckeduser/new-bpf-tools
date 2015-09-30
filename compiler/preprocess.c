/*
 * Preprocessor routine.
 */

#include <string.h>
#include <stdio.h>
#include "hashtable.h"
#include "general.h"
#include "preprocess.h"

extern int shutup_warnings;
extern char* current_file;

/*
 * e.g. 
 * #define DONALD(x,y) x+y
 */
typedef struct {
	char *name;		/* e.g. DONALD */
	char *body;		/* e.g. x+y */
	char argc;		/* e.g. 2 */
	char *argv[32];	/* e.g. &{"x","y"} */
} parameterized_macro_entry_t;

char result[2048];
char substituted_result[2048];

typedef struct {
	char *buf;
	int len;
	int alloc;
} extensible_buffer_t;

extensible_buffer_t* new_extensible_buffer(void)
{
	extensible_buffer_t* eb = malloc(sizeof(extensible_buffer_t));
	eb->buf = malloc(1024);
	eb->len = 0;
	eb->alloc = 1024;
	return eb;
}

void extensible_buffer_putchar(extensible_buffer_t* eb, char c)
{
	if (++eb->len >= eb->alloc) {
		eb->alloc *= 2;
		eb->buf = realloc(eb->buf, eb->alloc);
		if (!(eb->buf))
			fail("failed to realloc");
	}
	eb->buf[eb->len - 1] = c;
}

void linemarker(extensible_buffer_t *dest, int line_number, char *current_file)
{
	char buf[1024];
	char *p;
	sprintf(buf, "@linemark %s %d\n", current_file, line_number);
	for (p = buf; *p; ++p)
		extensible_buffer_putchar(dest, *p);
}

void preprocess(char **src, hashtab_t* defines)
{
	extern int iterate_preprocess(hashtab_t*, hashtab_t*, char**, int);
	int first = 1;

	hashtab_t *parameterized_defines = new_hashtab();

	while (iterate_preprocess(defines, parameterized_defines, src, first))
		if (first)
			first = !first;
}

void eatwhitespace(char **p)
{
	while (**p && (**p == ' ' || **p == '\t'))
		++*p;
}

void readlinetoken(char *dest, char **p)
{
	char *q;
	int prev = -1;
	int ignore = 0;
	for (q = dest; **p && **p != '\n'; ++*p) {
		/* // comment kills the token */
		if (prev > 0 && prev == '/' && **p == '/') {
			*--q = 0;
			ignore = 1;
		}
		if (!ignore) {
			*q++ = **p;
			prev = **p;
		}
	}
	*q = 0;
}

void readtoken(char *dest, char **p)
{
	char *q;
	for (q = dest; **p && **p != ' ' && **p != '\t' && **p != '\n'; ++*p)
		*q++ = **p;
	*q = 0;
}

void read_defkey_token(char *dest, char **p)
{
	char *q;
	int paren = 0;
	for (q = dest; **p && !(!paren && (**p == ' ' || **p == '\t')) && **p != '\n'; ++*p) {
		if (**p == '(')
			paren = 1;
		if (paren && **p == ')')
			paren = 0;
		*q++ = **p;
	}
	*q = 0;
}

int identchar(char c)
{
	return (c >= 'a' && c <= 'z')
		|| (c >= '0' && c <= '9')
		|| (c >= 'A' && c <= 'Z')
		|| (c == '_');
}

void readtoken2(char *dest, char **p)
{
	char *q;
	/*
	 * We want to split out either an identifier,
	 * or a non-identifier character alone.
	 */
	for (q = dest; **p && (**p != ' ' && **p != '\t' && **p != '\n') && identchar(**p); ++*p)
		*q++ = **p;
	if (q == dest && **p && (**p != ' ' && **p != '\t' && **p != '\n')) {
		*q++ = **p;
		++*p;
	}
	*q = 0;
}

char *preprocess_get_substituted_line(char **source_ptr_ptr, 
				      hashtab_t *defines, 
				      hashtab_t *parameterized_defines,
				      hashtab_t *macro_subst)
{
	char token[256];
	char *substitution;
	parameterized_macro_entry_t *param_subst;
	hashtab_t *macros;
	char *p = &result[0];
	char** argval/*[32][128];*/;
	char *q;
	char *r;
	int substitution_occured;
	int hash;
	int first;
	int in_string;
	int argn;
	char *result_copy, *result_copy_2, *current_copy, *source_copy;
	int i, j;
	char *lookahead;
	int depth = 0;
	int stringify;

	argval = malloc(128 * sizeof(char *));
	for (i = 0; i < 128; ++i)
		argval[i] = malloc(32 * sizeof(char));
	

	/*
	 * Copy the line to the line buffer
	 */
	while (**source_ptr_ptr && **source_ptr_ptr != '\n')
		*p++ = *((*source_ptr_ptr)++);
	if (**source_ptr_ptr && **source_ptr_ptr == '\n')
		*p++ = *((*source_ptr_ptr)++);

	/*
	 * Null-terminate the line in the line buffer
	 */
	*p = 0;

	/*
	 * Perform substitutions repeatedly
	 */
	first = 1;
	do {
		substitution_occured = 0;
		hash = 0;
		p = &result[0];
		q = &substituted_result[0];
		*q = 0;
		in_string = 0;
		while (*p) {
			stringify = 0;
			/*
			 * If this is a macro substitution we're doing
			 * and the next token is the ## glue operator,
			 * then the next occurence of whitespace will not be
			 * copied over.
			 */
			lookahead = p;
			eatwhitespace(&lookahead);
			if (macro_subst && *lookahead == '#' && lookahead[1] == '#')
				p = lookahead;
			/*
			 * Copying whitespace from the source
		 	 */
			while (*p && (*p == ' ' || *p == '\t' || *p == '\n')) {
				token[0] = *p;
				token[1] = 0;
				strcat(substituted_result, token);
				++p;
			}
			/*
			 * If it's a directive, do no substitutions
			 */
			if (first && *p == '#') {
				strcat(substituted_result, "#");
				hash = 1;
				++p;
			}
			/*
			 * Grab the next token that can be made
			 * of either a string of identifier characters
			 * or one non-identifier character. If it is
			 * made up of identifier characters, try to
			 * substitute it for a #define
			 */
get_token:
			*token = 0;
			readtoken2(&token[0], &p);
			if (*token && *token == '"'
			    && !(p != &result[0] && *(p-1) == '\\')) {
				in_string = !in_string;
			}
			/*
			 * Consume a ## glue operator and skip
			 * copying the whitespace after it if we're doing
			 * a macro substitution.
			 */
			if (macro_subst && !identchar(*token) && *token == '#' && *p == '#') {
				++p;
				eatwhitespace(&p);
				continue;
			}
			/*
			 * Stringification in macro substitutions
			 */
			if (macro_subst && !identchar(*token) && *token == '#') {
				stringify = 1;
				goto get_token;
			}
			/*
			 * At this point we would want to check
			 * if the token successfully looks up
			 * in a table of parameterized macros,
			 * then check for a left paren,
			 * then gobble up the list of arguments
			 * and so forth.
			 */
			if (!in_string && !hash && *token && identchar(*token)) {
				if((macro_subst && (substitution = hashtab_lookup(macro_subst, &token[0]))) 
				    || (substitution = hashtab_lookup(defines, &token[0]))) {
					/* avoid infinite loops in some sneaky cases */
					if (strcmp(token, substitution)) {
						substitution_occured = 1;
						if (stringify)
							strcat(substituted_result, "\"");
						strcat(substituted_result, substitution);
						if (stringify)
							strcat(substituted_result, "\"");
						continue;
					}
				} else if ((param_subst = hashtab_lookup(parameterized_defines, &token[0]))) {
					eatwhitespace(&p);
					if (*p != '(')
						fail("parenthesis expected after macro #define name");
					++p;
					argn = 0;
					depth = 0;
					while (*p && *p != '\n' && *p != ')') {
						r = &argval[argn][0];
						while (!((*p == ',' && depth == 0) || (*p==')' && depth == 0))) {
							if (*p == '(')
								++depth;
							if (*p == ')')
								--depth;
							*r++ = *p++;
						}
						*r = 0;
						if (*p == ',')
							++p;
						++argn;
					}
					if (*p == ')')
						++p;
					/*
					 * Make a new table with substitutions from
					 * the argument formal names to their values
					 * given here, and substitute the body of the 
					 * macro using it.
					 */
					
					result_copy = malloc(2048);
					current_copy = malloc(2048);
					source_copy = malloc(2048);
					strcpy(current_copy, substituted_result);
					strcpy(result_copy, param_subst->body);
					strcpy(source_copy, result);

					result_copy_2 = result_copy;

					macros = new_hashtab();

					for (i = 0; i < param_subst->argc; ++i)
						hashtab_insert(macros, 
							       param_subst->argv[i],
							       argval[i]);
					
					(void)preprocess_get_substituted_line(&result_copy_2,
									       defines,
									       parameterized_defines,
									       macros);

					strcat(current_copy, result);
					strcpy(result, source_copy);
					strcpy(substituted_result, current_copy);

					free(current_copy);
					free(result_copy);
					free(source_copy);

					continue;
				}
			}			
			if (*token)
				strcat(substituted_result, token);
			first = 0;
		}
		strcpy(result, substituted_result);
	} while (substitution_occured);


	for (i = 0; i < 128; ++i)
		free(argval[i]);
	free(argval);

	return &result[0];
}

int iterate_preprocess(hashtab_t *defines, 
		       hashtab_t *parameterized_defines,
		       char **src, int first_pass)
{
	int needs_further_preprocessing = 0;
	char *p;
	char *q;
	char *source_ptr;
	char *old_p;
	char *src_copy = my_strdup(*src);
	char directive[128];
	char error_message_buffer[256];
	char define_key[128];
	char define_val[256];
	char arg_name[256];
	char *arg_ptr;
	int line_number = 1;
	char include_file_buffer[128];
	char *include_file;
	FILE *include_file_ptr;
	char define_name[128];
	int define_exists;
	int depth;
	char *oldp;
	int keep;
	int i, j, k;
	int lex_state;
	parameterized_macro_entry_t macro_entry;
	parameterized_macro_entry_t *macro_entry_ptr;
	extensible_buffer_t *new_src = new_extensible_buffer();
	enum {
		INCLUDE_PATH_LOCAL,	/* "foo.h" */
		INCLUDE_PATH_SYSTEM	/* <foo.h> */
	} include_type;

	if (!src_copy)
		fail("failed to copy string");

	/*
	 * Line marker for first line
	 */
	if (first_pass)
		linemarker(new_src, line_number, current_file);

	for (source_ptr = src_copy; *source_ptr;) {
		/* consume whitespace at beginning of line */
		eatwhitespace(&source_ptr);

		/* fetch a new line */
		p = preprocess_get_substituted_line(&source_ptr, defines, parameterized_defines, NULL);

		/* check for a directive */
		if (*p == '#') {
			++p;	/* eat # */

			if (first_pass)
				linemarker(new_src, line_number, current_file);

			/* get the directive part */
			readtoken(directive, &p);

			#ifdef DEBUG
				fprintf(stderr, "CPP: directive: `%s'\n",
					directive);
			#endif
			
			/*
			 * XXX: TODO:
			 * #if, #elif, #error, #pragma, ...
			 */
			if (!strcmp(directive, "define")) {
				eatwhitespace(&p);
				read_defkey_token(define_key, &p);
				eatwhitespace(&p);
				readlinetoken(define_val, &p);
				/*
				 * But wait, is it a parameterized define ?
				 *
				 * On an unrelated note: about a month back, I went to the
				 * grillades restaurant, and after I had gazed 
				 * at the interesting kitchen instruments (there's a stove, 
				 * rotating meat brochette things, and like these flames in 
				 * the stove, and these pipes that are carrying what I suppose
				 * must be gas for the stove) for several minutes, 
				 * a man said: ``your sandwich is ready''. It was a good
				 * sandwich.
				 */
				if (strstr(define_key, "(") && strstr(define_key, ")")) {
					/*
					 * Yes, it is a parameterized define
					 */
					q = define_key;
					macro_entry.argc = 0;
					lex_state = 0;
					while (*q) {
						if (lex_state == 1) {
							if (*q == ',') {
								*arg_ptr = 0;
								macro_entry.argv[macro_entry.argc-1] = my_strdup(arg_name);
								++macro_entry.argc;
								arg_ptr = &arg_name[0];
								*arg_ptr = 0;
							} else if (identchar(*q)) {
								*arg_ptr++ = *q;
							}
						}
						if (*q == '(') {
							*q = 0;
							macro_entry.name = my_strdup(define_key);
							arg_ptr = &arg_name[0];
							*arg_ptr = 0;
							lex_state = 1;
							++macro_entry.argc;
						}
						if (*q == ')' && lex_state == 1) {
							*arg_ptr = 0;
							macro_entry.argv[macro_entry.argc-1] = my_strdup(arg_name);
							break;
						}
						++q;
					}
					macro_entry.body = my_strdup(define_val);
					macro_entry_ptr = malloc(sizeof(parameterized_macro_entry_t));
					*macro_entry_ptr = macro_entry;
					hashtab_insert(parameterized_defines, macro_entry.name, macro_entry_ptr);
				} else {
					/*
					 * No, it is not a parameterized define
					 */
					hashtab_insert(defines, define_key, my_strdup(define_val));
					#ifdef DEBUG
						fprintf(stderr, "CPP: define `%s' => `%s'\n",
							define_key, define_val);
					#endif
				}
			} else if (!strcmp(directive, "undef")) {
				eatwhitespace(&p);
				readtoken(define_key, &p);
				hashtab_pseudo_delete(defines, define_key);
				#ifdef DEBUG
					fprintf(stderr, "CPP: undefine `%s'\n",
						define_key);
				#endif
			} else if (!strcmp(directive, "include")) {
				eatwhitespace(&p);
				readtoken(include_file_buffer, &p);

				/*
				 * Figure out include path type,
				 * and trim specifier from path string
				 */
				if (*include_file_buffer == '"' || *include_file_buffer == '\'') {
					include_type = INCLUDE_PATH_LOCAL;
					include_file_buffer[strlen(include_file_buffer) - 1] = 0;
					include_file = include_file_buffer + 1;
				}
				else if (*include_file_buffer == '<') {
					include_type = INCLUDE_PATH_SYSTEM;
					include_file_buffer[strlen(include_file_buffer) - 1] = 0;
					include_file = include_file_buffer + 1;
				}
				else {
					include_type = INCLUDE_PATH_LOCAL;
					include_file = include_file_buffer;
				}

				if (include_type == INCLUDE_PATH_SYSTEM) {
					if (!shutup_warnings) {
						fprintf(stderr, "warning: ignoring <%s> system include\n",
							include_file);
					}
					goto skip_include;
				}

				needs_further_preprocessing = 1;

				include_file_ptr = fopen(include_file, "r");

				if (!include_file_ptr) {
					sprintf(error_message_buffer, 
						"%s: line %d: failed to open #include'd file `%s'", 
						current_file,
						line_number, 
						include_file);
					fail(error_message_buffer);
				}

				#ifdef DEBUG
					fprintf(stderr, "CPP: include `%s'\n",
						include_file);
				#endif

				linemarker(new_src, 1, include_file);
				for (;;) {
					int c = fgetc(include_file_ptr);
					if (c < 0)
						break;
					extensible_buffer_putchar(new_src, c);
				}
				if (first_pass)
					linemarker(new_src, line_number, current_file);
				fclose(include_file_ptr);

				skip_include:;

			} else if (!strcmp(directive, "ifdef") || !strcmp(directive, "ifndef")) {
				int negative = !strcmp(directive, "ifndef");
				enum { 
					INSIDE_IF,
					INSIDE_ELSE
				} current;

				eatwhitespace(&p);
				readtoken(define_name, &p);

				/* chomp to end of line */
				while (*p && *p != '\n')
					++p;
				++line_number;

				/* fetch a new line */
				p = preprocess_get_substituted_line(&source_ptr, defines, parameterized_defines, NULL);

				/*
				 * Here we only mind with the current depth,
				 * and let a recursive preprocessing sub-iteration
				 * handle the ifdef's at greater depth.
				 */
				needs_further_preprocessing = 1;
				depth = 1;

				define_exists = hashtab_lookup(defines, define_name) != NULL;
				current = INSIDE_IF;

				while (*p) {
					/* consume whitespace at beginning of line */
					eatwhitespace(&p);

					oldp = p;

					/* check for a directive */
					if (*p == '#') {
						++p;	/* eat # */

						if (first_pass)
							linemarker(new_src, line_number, current_file);

						/* get the directive part */
						readtoken(directive, &p);

						if (!strcmp(directive, "ifdef")) {
							++depth;
							p = oldp;
						} else if (!strcmp(directive, "ifndef")) {
							++depth;
							p = oldp;
						} else if (!strcmp(directive, "endif")) {
							if (!--depth)
								break;
							else
								p = oldp;
						} else if (!strcmp(directive, "else")) {
							if (depth == 1)
								current = INSIDE_ELSE;
							else
								p = oldp;
						} else {
							p = oldp;
						}
					}

					keep = (define_exists && current == INSIDE_IF)
						|| (!define_exists && current == INSIDE_ELSE);

					if (negative)
						keep = !keep;

					/* eat up till end of line */
					while (*p && *p != '\n') {
						if (keep)
							extensible_buffer_putchar(new_src, *p);
						++p;
					}
					if (*p == '\n') {
						if (keep)
							extensible_buffer_putchar(new_src, *p);
						++p;
						++line_number;
					}

					/* fetch a new line */
					p = preprocess_get_substituted_line(&source_ptr, defines, parameterized_defines, NULL);
				}

			} else {
				sprintf(error_message_buffer, 
					"warning: line %d: ignoring unsupported cpp directive `%s'\n", 
					line_number, 
					directive);
				/* fail(error_message_buffer); */
				fprintf(stderr, "%s", error_message_buffer);
				while (*p && *p != '\n')
					++p;
			}
		}

		/* eat up till end of line */
		while (*p && *p != '\n')
			extensible_buffer_putchar(new_src, *p++);
		if (*p == '\n') {
			extensible_buffer_putchar(new_src, *p++);
			++line_number;
		}
	}

	extensible_buffer_putchar(new_src, '\0');

	free(*src);
	*src = new_src->buf;

	return needs_further_preprocessing;
}

