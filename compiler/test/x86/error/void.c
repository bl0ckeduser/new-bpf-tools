void bob()
{
	return 123;
}

main()
{
	int x = bob();
}

/*
	Mon Oct 14 14:18:30 EDT 2013
	------------------------------------
	$ ./compile-run-x86.sh test/x86/error/void.c 
	void bob()
	     ^
	warning: line 1: only int- or char- sized return types work
	error: type conversion codegen fail

	------------------------------------
	A nicer error message might be nice

*****************************************************************************

	New error message as of Sat Oct 19 14:21:44 EDT 2013
	----------------------------------------------------

 int x = bob();
         ^
 error: line 8: data size mismatch -- something doesn't fit into something else

*/
