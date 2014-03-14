typedef struct match_struct {
	/* accept number, if any */
	int token;
	
	/* pointer to last character matched */
	char* pos;
} match_t;

match_t meta(int x, int y, int z)
{
	match_t match_algo(int);
	return match_algo(x + y + z);
}

match_t match_algo(int x)
{
	match_t m;
	m.pos = "hahaha";
	m.pos += x;
	return m;
}

main()
{
	/* match_t m = match_algo(); */
	match_t m = meta(1, 1, 1);
	printf("%s\n", m.pos);
}
