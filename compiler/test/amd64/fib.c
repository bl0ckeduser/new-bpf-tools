#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
int t = 5;
int a = 1;
int b = 0;

while (t-- > 0) {
	echo_int(a += b);
	echo_int(b += a);
}
}
