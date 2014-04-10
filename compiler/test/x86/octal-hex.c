#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
echo_int(0x20);
echo_int(0xFF);
echo_int(0xff);
echo_int(0xf3);
echo_int(0x3f);
echo_int(0xaF);
echo_int(0xAb);
echo_int(0xfb);
echo_int(0X32);

echo_int(012);
echo_int(021);
echo_int(011);
}
