#define DEBUG_VARIABLE(v) printf("The variable " #v " has value %d\n", v);

main()
{
	int donald = 1 + 2 * 3;

	printf("------------------------------------------\n");

	DEBUG_VARIABLE(donald);

	printf("------------------------------------------\n");
}
