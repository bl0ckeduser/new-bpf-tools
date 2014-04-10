#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
if (1 == 3) echo_int(0); else echo_int(1);
if (1 != 3) echo_int(1); else echo_int(0);

if (1 == 2) echo_int(0); else echo_int(1);
if (1 != 2) echo_int(1); else echo_int(0);

if (1 == 1) echo_int(1); else echo_int(0);
if (1 != 1) echo_int(0); else echo_int(1);

if (2 == 1) echo_int(0); else echo_int(1);
if (2 != 1) echo_int(1); else echo_int(0);

if (0 == 1) echo_int(0); else echo_int(1);
if (0 != 1) echo_int(1); else echo_int(0);

if (1 == 0) echo_int(0); else echo_int(1);
if (1 != 0) echo_int(1); else echo_int(0);

if (0 == 0) echo_int(1); else echo_int(0);
if (0 != 0) echo_int(0); else echo_int(1);
}
