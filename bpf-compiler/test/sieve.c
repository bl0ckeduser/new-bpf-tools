int MAX = 100;
int sieve[128];
int k;
int n;

sieve[1] = 1;
k = 2;
while (k < MAX)
	sieve[k++] = 0;

k = 1;
while(k++ < MAX){
    n = 2;
    while((n*k) <= MAX){
        sieve[n*k] = 1;
        n++;
    }
}

k = 0;
while(k++ < MAX){
    if(sieve[k] < 1){
        echo(k);
    }
}

