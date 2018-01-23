#include <stdio.h>
#define BOB(X,Y) bob_## X ## Y

bob_laughingclown() {
	printf("hahaha\n");
}

main()
{
	BOB(laughing,clown)();
}
