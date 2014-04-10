#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
if (2 <= 3) echo_int(1); else echo_int(0);
if (2 <= 2) echo_int(1); else echo_int(0);
if (2 <= 1) echo_int(0); else echo_int(1);

if (2 < 3) echo_int(1); else echo_int(0);
if (2 < 2) echo_int(0); else echo_int(1);
if (2 < 1) echo_int(0); else echo_int(1);

if (2 >= 3) echo_int(0); else echo_int(1);
if (2 >= 2) echo_int(1); else echo_int(0);
if (2 >= 1) echo_int(1); else echo_int(0);

if (2 > 3) echo_int(0); else echo_int(1);
if (2 > 2) echo_int(0); else echo_int(1);
if (2 > 1) echo_int(1); else echo_int(0);
}
