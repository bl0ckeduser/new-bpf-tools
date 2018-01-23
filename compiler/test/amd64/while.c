#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
int x = 5;

while (x > 0) {
	x -= 1;
	echo_int(x);
	echo_int(-x);
}
}
