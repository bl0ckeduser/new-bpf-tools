#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
int x;
int y;

x = 5;
echo_int(x--);
echo_int(y = x--);
echo_int(y);
echo_int(x);
echo_int(x--);
echo_int(y--);
}
