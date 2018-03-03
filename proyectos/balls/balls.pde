ArrayList<Ball> balls = new ArrayList<Ball>();


void setup() {
	size(1280, 720);
	strokeWeight(2);
	textSize(30);
}


void draw() {
	background(51);

	for (Ball a : balls) {
		for (Ball b : balls) {
			if (!a.equals(b) && a.hit(b)) {
				a.turn(b);
			}
		}
		a.update();
		a.show();
	}

	fill(255);
	text(int(frameRate) + "fps @" + balls.size(), 10, 35);

}

void mouseDragged() {
	Ball b = new Ball(mouseX, mouseY);
	for (Ball a : balls) {
		if (a.hit(b)) {
			return;
		}
	}
	balls.add(b);
}