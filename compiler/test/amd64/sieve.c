#include <stdio.h>
void echo_int(int n) { printf("%d\n", n); }

main() {
/*
 * Support for sequenced declarations like this one
 * has been added
 */
int MAX = 100, sieve[128], k, n;

sieve[1] = 1;
k = 2;
while (k < MAX)
	sieve[k++] = 0;

k = 1;
while(k++ < MAX){
    n = 2;
    while(n * k <= MAX){
        sieve[n*k] = 1;
        n++;
    }
}

k = 0;
while(k++ < MAX){
    if(sieve[k] < 1){
        echo_int(k);
    }
}

}
