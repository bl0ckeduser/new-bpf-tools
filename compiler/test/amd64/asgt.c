#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
int bob[100];

bob[12] = 0;

echo_int(bob[34] = bob[56] = bob[78] = 123);

echo_int(bob[12]);
echo_int(bob[34]);
echo_int(bob[56]);

echo_int(bob[12] += bob[34] += bob[56]);

echo_int(bob[12]);
echo_int(bob[34]);
echo_int(bob[56]);
}
