int ia[237];
char ca[3493];
int *ipa[49];

struct {
	int x;
	int y;
	char msg[123];
} foo;

struct bar_struct {
	int x;
	char msg[789];
	int y;
} *bar;
/* XXX: check against global struct pointer initializers ! */

struct bar_struct herp[32];

/* XXX: fails to compile properly */
/* typedef struct bar_struct bar_t; */
/* bar_t *derp[32]; */
struct bar_struct *derp[32];

main()
{
	int i, j;
	/*
	 * Test global arrays
	 */
	printf("int ");
	for (i = 230; i < 237; ++i) {
		ia[i] = (i - 230) * (i - 230);
		ipa[i - 230] = &ia[i];
	}
	for (i = 230; i < 237; ++i)
		printf("%d ", ia[i]);
	printf("\n");
	printf("int * ");
	for (i = 0; i < 7; ++i)
		printf("%d ", *ipa[i]);
	printf("\n");
	printf("char ");
	for (i = 3435; i < 3493; ++i)
		ca[i] = (i * i) % 26 + 'A';
	for (i = 3435; i < 3493; ++i)
		putchar(ca[i]);
	printf("\n");
	/*
	 * Test global structs
	 */
	foo.x = 123;
	foo.y = 456;
	strcpy(foo.msg, "hahahahaha");
	printf("%d %d %s\n", foo.x, foo.y, foo.msg);
	bar = malloc(sizeof(struct bar_struct));
	bar->x = 123;
	bar->y = 456;
	strcpy(bar->msg, "hahahahahahah !!!");
	printf("%d %d %s\n", bar->x, bar->y, bar->msg);
	for (i = 0; i < 32; ++i) {
		strcpy(herp[i].msg, "ha");
		printf("%s", herp[i].msg);
	}
	printf("\n");
	for (i = 0; i < 32; ++i) {
		derp[i] = malloc(sizeof(struct bar_struct));
		strcpy(derp[i]->msg, "ho");
		printf("%s", derp[i]->msg);
	}
	printf("\n");
}

