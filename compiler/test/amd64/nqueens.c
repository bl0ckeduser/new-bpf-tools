/*
 * A program that solves the N-queens problem
 * with N set to 8 for testing purposes
 *
 * I thought they were going to ask it as an interview question
 *
 * September 2014
 */

int nqueens(int *board, int N, int row) {
	int col;

	if (row == N)
		return checkboard(board, N);

	for (col = 0; col < N; ++col) {
		board[N * row + col] = 1;
		
		if (checkboard(board)
		    && nqueens(board, N, row + 1)) {
			return 1;
		} else {
			board[N * row + col] = 0;
		}
	}
	
	return 0;
}

int checkboard(int *board, int N) {
	int row, col;

	for (row = 0; row < N; ++row) {
		for (col = 0; col < N; ++col) {
			if (board[N*row+col]) {
				if (!checkqueenhorz(board, N, row, col)
				    || !checkqueenvert(board, N, row, col)
				    || !checkqueendiag(board, N, row, col)) {
					return 0;
				}
			}
		}
	}
	return 1;
}

int checkqueenhorz(int *board, int N, int row, int col) {
	int other_col;

	for (other_col = 0; other_col < N; ++other_col) {
		if (other_col != col &&
			board[N*row + other_col]) 
				return 0;
	}
	return 1;
}

int checkqueenvert(int *board, int N, int row, int col) {
	int other_row;

	for (other_row = 0; other_row < N; ++other_row) {
		if (other_row != row &&
			board[N*other_row + col]) 
				return 0;
	}
	return 1;
}

int checkqueendiag(int *board, int N, int row, int col) {
	int dist;

	for (dist = 0; dist < N; ++dist) {
		int ar = row - dist, ac = col - dist,
		    br = row - dist, bc = col + dist,
		    cr = row + dist, cc = col - dist,
		    dr = row + dist, dc = col + dist;

		/* this one choked the preprocessor before Sun Sep 28 14:30:12 EDT 2014 */

		#define CDH(r, c) checkdiaghelper(board, N, row, col, r, c)

		if (CDH(ar, ac) || CDH(br,bc) || CDH(cr,cc)
			|| CDH(dr, dc))
			return 0;
	}

	return 1;
}

int checkdiaghelper(int *board, int N, int row, int col,
			int other_row, int other_col) {

	return (row != other_row || col != other_col)
		&& range(other_row, N)
		&& range(other_col, N)
		&& board[N * other_row + other_col];

}

int range(int a, int b) {
	return a >= 0 && a < b;
}

void printboard(int *b, int N) {
	int row, col;

	for (row = 0; row < N; ++row) {
		for (col = 0; col < N; ++col) {
			printf("%c ", b[N * row + col] ? '*' : '_');
		}
		printf("\n");
	}
}

main(int argc, char **argv) {
	/* if (argc < 2) {
		printf("usage: %s N\n", *argv);
		return 1;
	} */

	int N;
	/* sscanf(argv[1], "%d", &N); */
	N = 8;
	int *board = malloc(N * N * sizeof(int));
	if (nqueens(board, N, 0)) {
		printboard(board, N);
	} else {
		printf("something has fucked up\n");
	}
}
