/*
 * Preprocessor routine.
 * Returns a hash table containing #define token mappings.
 * Modifies source text found at provided character pointer.
 */

#include <string.h>
#include <stdio.h>
#include "hashtable.h"
#include "general.h"
#include "preprocess.h"

extern char* current_file;

typedef struct {
	char *buf;
	int len;
	int alloc;
} extensible_buffer_t;

extensible_buffer_t* new_extensible_buffer()
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
	extern int iterate_preprocess(hashtab_t*, char**, int);
	int first = 1;

	while (iterate_preprocess(defines, src, first))
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
	for (q = dest; **p && **p != '\n'; ++*p)
		*q++ = **p;
	*q = 0;
}

void readtoken(char *dest, char **p)
{
	char *q;
	for (q = dest; **p && **p != ' ' && **p != '\t' && **p != '\n'; ++*p)
		*q++ = **p;
	*q = 0;
}

int iterate_preprocess(hashtab_t *defines, char **src, int first_pass)
{
	int needs_further_preprocessing = 0;
	char *p;
	char *q;
	char *old_p;
	char *src_copy = my_strdup(*src);
	char directive[128];
	char error_message_buffer[256];
	int line_number = 1;
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

	for (p = src_copy; *p;) {
		/* consume whitespace at beginning of line */
		eatwhitespace(&p);

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
			 * XXX: TODO: #define macro mode,
			 * #if, ##, ...
			 */
			if (!strcmp(directive, "define")) {
				char define_key[128];
				char define_val[256];
				eatwhitespace(&p);
				readtoken(define_key, &p);
				eatwhitespace(&p);
				readlinetoken(define_val, &p);
				hashtab_insert(defines, define_key, my_strdup(define_val));
				#ifdef DEBUG
					fprintf(stderr, "CPP: define `%s' => `%s'\n",
						define_key, define_val);
				#endif
			} else if (!strcmp(directive, "include")) {
				char include_file_buffer[128];
				char *include_file;
				FILE *include_file_ptr;

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
					fprintf(stderr, "warning: ignoring <%s> system include\n",
						include_file);
					goto skip_include;
				}

				needs_further_preprocessing = 1;

				include_file_ptr = fopen(include_file, "r");

				if (!include_file_ptr) {
					sprintf(error_message_buffer, 
						"line %d: failed to open #include'd file `%s'", 
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
				char define_name[128];
				int define_exists;
				int depth;
				char *oldp;
				int keep;
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

/*
	./wcc -DTEST_CPP preprocess.c hashtable.c general.c -o testcpp
	./testcpp

	yes indeed, partial selfcompilation
*/
#ifdef TEST_CPP
	int main(int argc, char **argv)
	{
		hashtab_t *tab = new_hashtab();
		char *foo = my_strdup( 
			"#define FOO 1 + 2 * 3\nint x = FOO;\n"
			"#define THING\n"
			"#include \"bob.h\""
		);
		(void)preprocess(&foo, tab);
		printf("-----------------\n");
		puts(foo);
	}
#endif
