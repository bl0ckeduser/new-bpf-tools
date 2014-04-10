#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
int i;
int j;

i = 1;
while (i <= 4) {
	j = 1;
	while (j <= 4)
		echo_int(i * j++);
	++i;
}
}
