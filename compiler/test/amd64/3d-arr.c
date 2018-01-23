/* 3D array with declared dimensions */
main() {
	int box[5][6][7];
	int i, j, k;

	for (i = 0; i < 5; ++i)
		for (j = 0; j < 6; ++j)
			for (k = 0; k < 7; ++k)
				box[i][j][k] = i * j * k;
	printf("write ok\n");

	for (i = 0; i < 5; ++i)
		for (j = 0; j < 6; ++j)
			for (k = 0; k < 7; ++k)
				printf("%d*%d*%d=%d\n",
					i, j, k, box[i][j][k]);
	printf("read ok\n");
}

