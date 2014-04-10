#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
// Arithmetic is left-associative !
echo_int(10 / 5/ 2);
echo_int(20 / 2 / 2 / 2 / 2);
echo_int(5 - 1 - 1 - 1 - 1 - 1);
echo_int(3 - 2 - 1);

// Let's still check that these compile right
echo_int(10/(5/2));
echo_int(20 / 2 / (2 / 2) / 2);
echo_int(5 - 1 - 1 - (1 - 1 - 1));
echo_int(3 - (2 - 1));

}
