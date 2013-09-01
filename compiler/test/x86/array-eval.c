main()
{
	char buf[1024];
	char *p;

	strcpy(buf, "hello");

	/*
	 * this is implicitly taken to mean p = &buf[0]
	 * by the compiler because of the way C implements
	 * arrays
	 */
	p = buf;

	puts(p);
}
