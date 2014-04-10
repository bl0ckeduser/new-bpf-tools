#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main(){
int x;

x = 1 + 2 * 3;	// proper precedence !

x *= 3 + 4;

echo_int(x * 5 + 3 * 4);

int clown;
;;;;/* ... empty statements ... */;;;;;;;;;;;;;;
clown = 123;
clown *= clown + /* herp */ 1;	// derp
// clown *= 123456789
echo_int(/*-*/clown + 2);

	;	;

echo_int(-3 + 5);
echo_int(-123);
}
