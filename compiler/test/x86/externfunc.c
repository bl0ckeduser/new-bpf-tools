extern int putchar(int c);

main()
{
	char *message = "heya consider them checked\n";
	--message;
	while (*++message)
		putchar(*message);
}
