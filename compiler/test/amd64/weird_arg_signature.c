/*
 * a weird dark corner of C, not sure what it is supposed to do
 * gcc just seems to silent accept it
 */

void foo(int haha[1+2*3])
{
	printf("%d\n", haha[0]);
}

main()
{
	int bar[] = {123,456,789};
	foo(bar);
}
