main(argc, a_argv) {
	int i;
	char **argv = (char **)a_argv;
	for (i = 0; i < argc; ++i)
		puts(argv[i]);
}
