/* escape sequence -> character	*/
/* Source: http://web.cs.mun.ca/~michael/c/ascii-table.html */

/* XXX:	this isn't smart enough	to deal	with octal sequences like \012
 * in fact even	the interface (one character) is too stupid to start
 * with	*/

extern void fail(char*);

int escape_code(char c)
{
	switch (c) {
		case '0':	return '\0';
		case 'a':	return '\a';
		case 'b':	return '\b';
		case 't':	return '\t';
		case 'f':	return '\f';
		case 'n':	return '\n';
		case 'r':	return '\r';
		case '\\':	return '\\';	/* \\	*/
		case '\'':	return '\'';	/* \'	*/
	}

	fail("invalid escape sequence");
}

