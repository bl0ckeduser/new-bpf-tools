typedef struct herp_derp {
	int x;
	char foo[59];
	int y;
} herp_derp_t;

main()
{
	int bob;
	int arr[32];
	int matrix[8][8];
	struct herp_derp *ptr;
	struct herp_derp arr2[32];

	printf("sizeof(int) - sizeof(bob) = %d\n", sizeof(int) - sizeof(bob));
	printf("sizeof(int) / sizeof(bob) = %d\n", sizeof(int) / sizeof(bob));
	printf("sizeof(arr) / sizeof(int) = %d\n", sizeof(arr) / sizeof(int));
	printf("sizeof(matrix) / sizeof(int) = %d\n", sizeof(matrix) / sizeof(int));

	ptr = malloc(sizeof(struct herp_derp));
	strcpy(ptr->foo,
		"Well if all of this comes out properly the malloc went ok");
	puts(ptr->foo);

	printf("sizeof(arr2) / sizeof(struct herp_derp)) = %d\n",
		sizeof(arr2) / sizeof(struct herp_derp));

	/* XXX: sizeof(a typedef) is BROKEN */
	/* printf("sizeof(arr2) / sizeof(herp_derp_t)) = %d\n",
		sizeof(arr2) / sizeof(herp_derp_t)); */
}
