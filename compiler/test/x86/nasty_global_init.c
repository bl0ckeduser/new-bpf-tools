char out[52];
int iout[52];
void *outPointer = &out;
void *outPointer2 = &out[4];
void *outPointer3 = &out[1 + 2 * 3];

void *ioutPointer = &iout;
void *ioutPointer2 = &iout[4];
void *ioutPointer3 = &iout[1 + 2 * 3];

int donald = 3;
int* bob = &donald;

main()
{
	out[0] = 'X';
	out[4] = 'Y';
	out[7] = 'Z';

	iout[0] = 1234;
	iout[4] = 56789;
	iout[7] = 10;

	printf("%c", 	*(char *)outPointer);
	printf("%c", 	*(char *)outPointer2);
	printf("%c\n", 	*(char *)outPointer3);

	printf("%d", 	*(int *)ioutPointer);
	printf("%d ", 	*(int *)ioutPointer2);
	printf("%d\n", 	*(int *)ioutPointer3);

}
