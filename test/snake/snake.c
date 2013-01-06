// Simple "snake" game (uses keyboard)
// by blockeduser Dec. 23, 2012
// Updated Jan. 5, 2013 to use for-statements

game_start:

int TAIL_LENGTH = 16;	// pretty arbitrary
int x = 7 * 16;
int y = 7 * 16;
int xv = 0;
int yv = 16;
int kbcheck;
int temp;
int kb_bits[8];
int i;
int snake_tail_x[32];
int snake_tail_y[32];
int tail_len = 0;
int tail_ptr = 0;
int game_over = 0;

/* if keyboard isn't supported,
 * put up an error message and hang */
_kbreg = 240;
if (_kbreg != 123) {
	draw (0, 0, 255, 255, 17, 41, 10, 10);	// white bg
	draw(10, 100, 252, 51, 2, 186, 252, 51);	// message
	outputdraw();
	for (;;)
		wait(1, 10);
}

for (;;) {
	kbcheck = _kbreg;	// read the keyboard byte
	/* now split it up into bits.
	 * (no, the vm doesn't provide bit ops or
	 * even modulo. this is why this is fun) */
	for (i = 0; i < 8; ++i) {
		temp = kbcheck;
		while (temp >= 2)
			temp -= 2;
		kbcheck /= 2;
		kb_bits[i] = temp;
	}

	if (game_over == 0) {
		/* "else if" is used to make it only
		 * possible to go in one direction at once
		 */

		if (kb_bits[4] == 1) {		// right key
			xv = 16;
			yv = 0;
		} else if (kb_bits[5] == 1) {	// left key
			xv = -16;
			yv = 0;
		} else if (kb_bits[6] == 1) {	// down key
			xv = 0;
			yv = 16;
		} else if (kb_bits[7] == 1) {	// up key
			xv = 0;
			yv = -16;
		}

		// store old tail coordinates in a ring buffer
		if (tail_len < TAIL_LENGTH)
			++tail_len;
		snake_tail_x[tail_ptr] = x;
		snake_tail_y[tail_ptr] = y;
		if (++tail_ptr >= TAIL_LENGTH)
			tail_ptr = 0;

		// move snake head
		x += xv;
		y += yv;

		// check if snake is within screen bounds
		if (x < 0)
			game_over = 1;
		if (x > 255)
			game_over = 1;
		if (y < 0)
			game_over = 1;
		if (y > 255)
			game_over = 1;
	}

	draw (0, 0, 255, 255, 17, 41, 10, 10);	// white bg
	draw(x, y, 16, 16, 153, 33, 10, 10);	// draw head

	/* draw the snake tail and check for collision
	 * with it */
	if (tail_len < TAIL_LENGTH)
		temp = 0;
	else
		temp = tail_ptr;
	for (i = 0; i < tail_len; ++i) {
		draw(snake_tail_x[temp], snake_tail_y[temp],
			16, 16, 153, 33, 10, 10);

		if (x == snake_tail_x[temp])
			if (y == snake_tail_y[temp])
				if (game_over == 0)
					game_over = 1;

		if (++temp >= TAIL_LENGTH)
			temp = 0;
	}

	if (game_over == 1) {
		draw(58, 100, 121, 27, 7, 147, 121, 27); // "game over"
		// reset game when user presses enter
		if (kb_bits[1] == 1)
			goto game_start;
	}

	outputdraw();
	wait(1, 15);
}
