/*
 * This is a solution to the problem "Problem S5: Mouse Journey"
 * which was given at the first stage of the 2012 edition of the
 * Canadian Computing Competition. I wrote this code on Feb. 12, 2013
 * while practicing for the 2013 edition of the competiton.
 *
 * The problem description is given in:
 * http://cemc.uwaterloo.ca/contests/computing/2012/stage1/seniorEn.pdf
 *
 * And Waterloo provides test cases at:
 * http://cemc.uwaterloo.ca/contests/computing/2012/stage1/Stage1Data/UNIX_OR_MAC.zip
 *
 * I wrote a script to try the test cases, it's called "ccc2012-s5-test.sh";
 * it's in this directory, it has to be run from this directory, and it needs
 * some tweaking before it actually works.
 */

typedef struct node {
    struct node* r;
    struct node* d;
    int val;
} node_t;

int accumulate(node_t *nod)
{
    int sum = 0;
    
    /* base case */
    if (nod->val >= 0)
        return nod->val;
    
    /* otherwise */
    if (nod->r)
        sum += accumulate(nod->r);
    if (nod->d)
        sum += accumulate(nod->d);
    return nod->val = sum;
}

main()
{
    int i, j;
    int r, c;
    int c_r, c_c;
    int cc = 0;
    node_t** map;

    map = malloc(30 * sizeof(node_t*));
    for (i = 0 ; i < 30; ++i)
        map[i] = malloc(30 * sizeof(node_t));
    
    
    /* read row, column count */
    scanf("%d %d", &r, &c);
    
    /* read cat count */
    scanf("%d", &cc);
    
    /* build the initial tree */
    for (i = 1; i <= r; ++i) {
        for (j = 1; j <= c; ++j) {
            map[i][j].val = -1;
            map[i][j].r = map[i][j].d = 0x0; /* XXX: NULL unsupported */
            if (i < r) /* link to below if possible */
                map[i][j].d = &(map[i + 1][j]);
            if (j < c) /* link to right if possible */
                map[i][j].r = &(map[i][j + 1]);
        }
    }
    
    map[r][c].val = 1; /* ending point */
    for (i = 0; i < cc; ++i) {
        scanf("%d %d", &c_r, &c_c);
        map[c_r][c_c].val = 0; /* set cat */
    }
    
    /* accumulate values */
    printf("%d\n", accumulate(&(map[1][1])));
    
    return 0;
}
