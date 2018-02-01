void func(int arg0, int arg1, int arg2, int arg3, int arg4,
	  int arg5, int arg6, int arg7, int arg8, int arg9,
	  int argA, int argB, int argC, int argD, int argE)
{
	printf("%x\n", arg0);
	printf("%x\n", arg1);
	printf("%x\n", arg2);
	printf("%x\n", arg3);
	printf("%x\n", arg4);
	printf("%x\n", arg5);
BUG:	printf("%x\n", arg6);
	printf("%x\n", arg7);
	printf("%x\n", arg8);
	printf("%x\n", arg9);
	printf("%x\n", argB);
	printf("%x\n", argC);
	printf("%x\n", argE);
}

main()
{
	func(0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
	     0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE);
}
