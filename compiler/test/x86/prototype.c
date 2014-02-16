/* a prototype */
int* toothlessness(int,
		   char* ronald_mcdonald_is_named_donald_in_japan);

/* a prototype with an empty arglist (these didn't use to work) */
int foo();

/* (void) means exactly zero arguments, as far as I understand */
int bar(void);

foo()
{
	puts("hahahahahaah");
}

bar(void)
{
	puts("hohohohohohoh");
}


int main(int argc, char* argv[])
{
	int* hamburgers_the_cornerstone_of_any_nutrititous_breakfast
		= toothlessness(123,
			"HAHAHA lol I didn't know that ! With A D you say!");

	printf("%s\n",
		"English, motherfucker, do you speak it ?");

	printf("*hambugers = %d\n", 
		*hamburgers_the_cornerstone_of_any_nutrititous_breakfast);

	foo();
	bar();
}

int* toothlessness(int foo, char* reaction_to_weird_fact) {
	int* bagel = malloc(sizeof(int));
	*bagel = 456;
	return bagel;
}
