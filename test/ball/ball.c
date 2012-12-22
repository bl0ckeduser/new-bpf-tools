int ball_x = 90;
int ball_y = 100;
int ball_xvel = 5;
int ball_yvel = 5;

draw(0, 0, 255, 255, 119, 219, 10, 10);

while (1 == 1) {
	ball_x += ball_xvel;
	ball_y += ball_yvel;
	
	if (ball_x > 235 - 20) {
		ball_x = 235 - 20;
		ball_xvel *= -1;
	}
	if (ball_x < 20) {
		ball_x = 20;
		ball_xvel *= -1;
	}
	if (ball_y > 195 - 20) {
		ball_y = 195 - 20;
		ball_yvel *= -1;
	}
	if (ball_y < 60) {
		ball_y = 60;
		ball_yvel *= -1;
	}

	draw(0, 0, 255, 212, 0, 0, 255, 212);
	draw(ball_x, ball_y, 20, 20, 28, 216, 20, 20);
	outputdraw();

	wait(1, 30);
}


