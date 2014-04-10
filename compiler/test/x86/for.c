#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
int i = 3;

for (;;) {
	echo_int(i--);
	if (i < 0)
		goto hahaha;
}

hahaha:

for (i = 0; ;++i) {
	echo_int(i);
	if (i > 5)
		goto hohoho;
}

hohoho:

for (i = 0; i < 5; ++i)
	echo_int(++i);

}
