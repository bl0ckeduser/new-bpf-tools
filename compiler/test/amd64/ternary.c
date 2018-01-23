test(int a, int b)
{
	printf("%d%d\n", a, b);
}

main()
{
	int arr[5], i;
	printf("%d\n", 1 ? 123 : 456);
	printf("%d\n", 0 ? 123 : 456);
	printf("%s\n", 1 ? "foo" : "bar");
	printf("%s\n", 0 ? "foo" : "bar");
	arr[0] = arr[1] = arr[2] = arr[3] = 0;
	while (!arr[3]) {
		printf("%d ? %d : %d ", arr[0], arr[1], arr[2]);
		printf(" => %d\n", arr[0] ? arr[1] : arr[2]);
		++*arr;
		for (i = 0; i < 4; ++i) {
			if (arr[i] > 1) {
				arr[i] = 0;
				arr[i + 1]++;
			}
		}
	}

	test(1 ? 123 : 0, 456);
}
