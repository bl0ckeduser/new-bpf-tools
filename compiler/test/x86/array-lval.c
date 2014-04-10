#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
int bob[10];

bob[5] = 7;
bob[3] = 12;
++bob[5];
echo_int(++bob[5]);
echo_int(bob[5]++);
while (++bob[5] < 32) {
	echo_int(bob[5]++);
	echo_int(--bob[3]);
	bob[5] += 12;
}

}
