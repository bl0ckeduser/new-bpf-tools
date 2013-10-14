/*
 * This is a solution to the problem "Problem S2: Huffman Encoding"
 * which was given at the first stage of the 2010 edition of the
 * Canadian Computing Competition. I wrote this code on Feb. 15, 2013
 * while practicing for the 2013 edition of the competiton.
 * (The changes from the original C version, other than removing
 * standard library preprocessor #includes, are marked with XXX).
 *
 * The problem description is given in:
 * http://cemc.uwaterloo.ca/contests/computing/2010/stage1/seniorEn.pdf
 *
 * And Waterloo provides test cases at:
 * http://cemc.uwaterloo.ca/contests/computing/2010/stage1/Stage1Data/UNIX_OR_MAC.zip
 *
 * I wrote a script to try the test cases, it's called "ccc2010-s2-test.sh";
 * it's in this directory, it has to be run from this directory, and it needs
 * some tweaking before it actually works.
 */

#include <stdio.h>

typedef struct node {
	struct node* l;
	struct node* r;
	char val;
	int has_val;
} node_t;

node_t* new_node() {
	node_t* n = malloc(sizeof(node_t));
	if (!n) {
		printf("memory failure\n");
		exit(1);
	}
	n->has_val = 0;
	n->l = n->r = /* XXX: NULL */ 0x0;
	return n;
}

main()
{
	int i;
	int n;
	char buf[1024];
	char patt[1024];
	char c;
	char *p;
	node_t *root = new_node();
	node_t *nod;

	fgets(buf, 1024, stdin);
	sscanf(buf, "%d", &n);

	for (i = 0; i < n; ++i) {
		fgets(buf, 1024, stdin);
		sscanf(buf, "%c %s", &c, patt);
		for (p = patt, nod = root; *p; ++p) {
			if (*p == '0') {
				if (!nod->l)
					nod->l = new_node();
				nod = nod->l;
			} else if (*p == '1') {
				if (!nod->r)
					nod->r = new_node();
				nod = nod->r;
			}
		}
		nod->has_val = 1;
		nod->val = c;
	}

	fgets(patt, 1024, stdin);
	for (p = patt, nod = root; *p; ++p) {
		if (*p == '0') {
			if (nod->l)
				nod = nod->l;
			else
				nod = root;
		} else if(*p == '1') {
			if (nod->r)
				nod = nod->r;
			else
				nod = root;
		}
		if (nod->has_val) {
			putchar(nod->val);
			nod = root;
		}
	}
	putchar('\n');
}
