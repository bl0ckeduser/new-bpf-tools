/*
 * This code was originally written as homework for an
 * Intro to Unix and C university course, (which proved
 * mostly useless), on February 4, 2014.
 *
 * Now, it tests a new change in the x86 code generator
 * Previously it would run out of registers trying to
 * code the encrypt() routine because dereferences
 * were being done in a rather sub-optimal way.
 */

/*
 * Caesar cypher
 */
void encrypt(char *buf, int number)
{
        for (; *buf; ++buf) {
                if (*buf >= 'A' && *buf <= 'Z') 
                        *buf = (*buf - 'A' + number) % 26 + 'A';
                if (*buf >= 'a' && *buf <= 'z')
                        *buf = (*buf - 'a' + number) % 26 + 'a';
        }
}


main()
{
	char* message = strdup("hello world");
	encrypt(message, 13);
	puts(message);
}
