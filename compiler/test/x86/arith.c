main(){
int x;

x = 1 + 2 * 3;	// proper precedence !

x *= 3 + 4;

echo(x * 5 + 3 * 4);

int clown;
;;;;/* ... empty statements ... */;;;;;;;;;;;;;;
clown = 123;
clown *= clown + /* herp */ 1;	// derp
// clown *= 123456789
echo(/*-*/clown + 2);

	;	;

echo(-3 + 5);
echo(-123);
}
