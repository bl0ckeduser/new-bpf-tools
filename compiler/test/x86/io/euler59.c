/*
 * Solution to projecteuler problem #59 ("XOR decryption"),
 * based on a real-C version written on June 25, 2012.
 *
 * This program requires the file "cipher1.txt"
 * available from http://projecteuler.net/project/cipher1.txt
 * and an english dictionary in the file "words.txt",
 * in the usual style where there is one word per line.
 */

typedef struct trie_s {
	struct trie_s* map[256];
	int entry;
} trie;

trie *trie_node()
{
	trie* t = malloc(sizeof(trie));
	int i;

	if(!t) return 0;
	t->entry = 0;
	for(i = 0; i < 256; i++)
		t->map[i] = 0;

	return t;	
}

void error(char* what)
{
	printf("error: %s\n", what);
	exit(1);
}

int main(int argc, char** argv)
{
	void* input = fopen("cipher1.txt", "r");
	void* words = fopen("words.txt", "r");
	int c, n;
	char* cipher = malloc(1024 * 1024 * 100);
	char* p;
	char buf[1024] = "";
	char best_passwd[10] = "failed";
	int pass[4];
	int i = 0;
	int cipher_length;
	int j;
	trie* t = trie_node();
	trie* node;
	int wc;
	int record = 0;
	int sum;

	if(!input) error("could not open cipher file");
	if(!words) error("could not open words file");
	if(!t)	error("could not create trie");

	printf("reading in cipher\n");
	while(!feof(input)) {
		p = buf;
		while((c = fgetc(input)) != ',' && c >= 0) {
			*(p++) = c;
		}
		*p = 0;
		sscanf(buf, "%d", &n);
		cipher[i++] = n;
		printf("got %d -> %d\n", n, i);
	}
	cipher_length = i;

	printf("reading in words\n");
	while(1) {
		if(fgets(buf, 1024, words) == 0) break;
		p = buf;
		node = t;
		while(*p && *p != '\n') {
			if(!node->map[*p])
				if(!(node->map[*p] = trie_node()))
					error("trie node allocation");
			node = node->map[*p];
			++p;
		}
		node->entry = 1;		
	}

	pass[0] = pass[1] = pass[2] = 'a';
	pass[3] = 0;
	while(!pass[3]) {
		for(i = 0; i <= 3; i++)
			if(pass[i] > 'z')
			{
				pass[i] = 'a';
				pass[i+1]++;
			}

		node = t;

		wc = 0;
		for(i = 0; i < cipher_length; i++) {
			if(node->map[cipher[i] ^ (pass[i % 3])] != 0
			 && isalnum(cipher[i] ^ (pass[i % 3]))) {
				node = node->map[cipher[i] ^ (pass[i % 3])];
				*p++ = (cipher[i] ^ (pass[i % 3]));
				*p = 0;			
			}
			else
				goto clear_up;

			if(node->entry && strlen(buf) > 3) {
				++wc;
				printf("password %c%c%c; found word: %s\n", *pass, pass[1], pass[2], buf);
clear_up:
				*buf = 0;
				p = buf;
				node = t;
			}
		}

		if(wc > record) {
			record = wc;
			sprintf(best_passwd, "%c%c%c", *pass, pass[1], pass[2]);
		}

		++*pass;
	}

	printf("best password found was '%s', which yielded %d wordfinds.\n", best_passwd, record);
	printf("decrypted message:\n");
	
	for(i = 0; i < cipher_length; i++)
		putchar(cipher[i] ^ best_passwd[i % 3]);
	printf("\n");

	sum = 0;
	for(i = 0; i < cipher_length; i++)
		sum += cipher[i] ^ best_passwd[i % 3];
	
	printf("ascii sum: %d\n", sum);
	return 0;	
}
