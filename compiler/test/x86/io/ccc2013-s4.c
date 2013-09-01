/*
 * This is a solution to the problem "Problem S4: Who is taller?"
 * which was given at the first stage of the 2013 edition of the
 * Canadian Computing Competition. I wrote this code on Feb. 26, 2013
 * while participating in the 2013 edition of the competiton.
 *
 * The original C version had some differences; it was modified
 * because of limitiations in this wannabe compiler. Also, it had
 * a bug that gcc didn't bother to crash on.
 * The original version of the code is available at:
 * http://sites.google.com/site/bl0ckeduserssoftware/s4-pub.c
 *
 * The problem description is given in:
 * http://cemc.uwaterloo.ca/contests/computing/2013/stage1/seniorEn.pdf
 *
 * And Waterloo provides test cases at:
 * http://cemc.uwaterloo.ca/contests/computing/2013/stage1/Stage1Data/UNIX_OR_MAC.zip
 *
 * I wrote a script to try the test cases, it's called "ccc2013-s4-test.sh";
 * it's in this directory, it has to be run from this directory, and it needs
 * some tweaking before it actually works.
 */

// mishmash of linked list and table
// with memoization (derp it's gotta be fast)

char **memo_tab;

typedef struct derp_s {
	int* tt;
	int cnt;
	int alloc;
} derp_t;

derp_t** lists;

char *my_realloc(char *ptr, int len)
{
        char *new_ptr = malloc(len);
        int i;
		if (ptr) {
	        for (i = 0; i < len; ++i)
	                new_ptr[i] = ptr[i];
	        free(ptr);
		}
        return new_ptr;
}

/* XXX: void expand(int d, int siz); */

derp_t* new_derp(int id)
{
	derp_t* d = malloc(sizeof(derp_t));
	d->tt = malloc(1 * sizeof(int *));
	d->alloc = 1;
	d->cnt = 0;
	/* this "return" line was missing from
	 * the original C code ! */
	return d;
}

int look_for(int r, int c)
{
	int i;
	int rr;
	
	// memoize for r, c < 100
	if (r < 100 && c < 100) {
		if (memo_tab[r][c]) {
			return memo_tab[r][c] - 1;
		}
		else {
			
			rr = look_for_raw(r, c);
			memo_tab[r][c] = rr + 1;
			return rr;
		}
	}
	else
		return look_for_raw(r, c);
}

int look_for_raw(int r, int c)
{
	int i;
	
	if (check_child(r, c))
		return 1;
		
	for (i = 0; i < lists[r]->cnt; ++i)
		if (look_for(lists[r]->tt[i], c))
			return 1;
			
	return 0;
}

int check_child(int father, int child)
{
	int i =0;
	for (i = 0; i < lists[father]->cnt; ++i)
		if (lists[father]->tt[i] == child) {
			return 1;
		}
	return 0; // no !
}

/* XXX: void */ add_child(int dest, int child)
{
	if (++(lists[dest]->cnt) >= lists[dest]->alloc)
		expand(dest, lists[dest]->cnt);
	lists[dest]->tt[lists[dest]->cnt - 1] = child;
}

/* XXX: void */ expand(int d, int siz)
{
	while (lists[d]->alloc < siz)
		lists[d]->alloc += 16;
	/* XXX: realloc */
	lists[d]->tt = my_realloc(lists[d]->tt, lists[d]->alloc * sizeof(int));
	if (!(lists[d]->tt)) {
		printf("aw snap, out of heap memory :(\ngonna abort now\n");
		exit(1);
	}
}

main()
{
	char buf[1024];
	int people;
	int comps;
	int i = 0;
	int j = 0;
	int a, b;
	int ma, mb; // the mysterious ones
	
	// setup the memoization table
	memo_tab = malloc(100 * sizeof(char *));
	for (i = 0; i < 100; ++i) {
		memo_tab[i] = malloc(100);
		for (j = 0; j < 100; ++j)
			memo_tab[i][j] = 0;
	}
	
	/* XXX: fgets(buf, 1024, stdin); */
	gets(buf);
	sscanf(buf, "%d %d", &people, &comps);
	
	lists = malloc((people+1) * sizeof(derp_t *));
	if (!lists) {
		printf("aw snap, malloc failed\n");
		exit(1);
	}
	
	for (i = 0; i < people + 1; ++i)
		lists[i] = new_derp(i);
	
	// set up the list from known info
	for (i = 0; i < comps; ++i) {
		/* XXX: fgets(buf, 1024, stdin); */
		gets(buf);
		sscanf(buf, "%d %d", &a, &b);
		//printf("k so %d is taller than %d kthxbye\n", a, b);
		add_child(a, b);
	}
	
	// now solve the mystery lololol
	/* XXX: fgets(buf, 1024, stdin); */
	gets(buf);
	sscanf(buf, "%d %d", &ma, &mb);
	if (look_for(ma, mb))
		printf("yes\n");
	else if (look_for(mb, ma))
		printf("no\n");
	else
		printf("unknown\n");
	
	return 0;
}
