derp(char *ptr)
{
	int index = 1;
	printf(">> %d\n", ptr[1]);
	printf(">> %d\n", *(ptr + index));
}

main()
{
	char buf[1024];
	buf[1] = 'A';
	printf("%c\n", buf[1]);
	derp(buf);
}
