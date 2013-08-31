typedef struct herp_derp {
	int x;
	char foo[59];
	int y;
} herp_derp_t;

typedef int mystery_type;
typedef mystery_type meta_typedef;

/* XXX: typedef is not that smart */
/* typedef meta_typedef* pointer; */

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

	printf("sizeof(arr2) / sizeof(herp_derp_t)) = %d\n",
		sizeof(arr2) / sizeof(herp_derp_t));

	printf("sizeof(struct herp_derp) / sizeof(herp_derp_t)) = %d\n",
		sizeof(struct herp_derp) / sizeof(herp_derp_t));

	printf("sizeof(struct herp_derp **) / sizeof(herp_derp_t **)) = %d\n",
		sizeof(struct herp_derp **) / sizeof(herp_derp_t **));

	printf("sizeof(mystery_type) / sizeof(int) = %d\n",
		sizeof(mystery_type) / sizeof(int));

	printf("sizeof(meta_typedef) / sizeof(int) = %d\n",
		sizeof(meta_typedef) / sizeof(int));

/*
	printf("sizeof(pointer) / sizeof(int *)\n",
		sizeof(pointer) / sizeof(int *));
*/
}
