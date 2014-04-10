#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
	echo_int(12);
	goto hax;
bob:
	echo_int(56);
	goto end;
hax:
	echo_int(34);
	goto bob;
end:
	;;;

}
