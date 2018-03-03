class Ball {
	PVector pos;
	PVector dir;
	float r = random(5, 50);
	int col[] = new int[3];

	Ball(float x, float y) {
		pos = new PVector(x, y);
		dir = new PVector(random(-10, 10), random(-10, 10));
		col[0] = (int)random(0, 255);
		col[1] = (int)random(0, 255);
		col[2] = (int)random(0, 255);
	}

	void update() {
		if ((dir.x > 0 && pos.x + r >= width) || (dir.x < 0 && pos.x - r <= 0)) {
			dir.x *= -1;
		}
		if ((dir.y > 0 && pos.y + r >= height) || (dir.y < 0 && pos.y - r <= 0)) {
			dir.y *= -1;
		}
		pos.add(dir);
	}

	void show() {
		fill(col[0], col[1], col[2]);
		ellipse(pos.x, pos.y, r * 2, r * 2);
	}

	boolean hit(Ball b) {
		float distX = pos.x - b.pos.x;
		float distY = pos.y - b.pos.y;
		float distance = sqrt((distX * distX) + (distY * distY));

		return distance <= r + b.r;
	}

	void turn(Ball b) {
		float aux = dir.x;
		dir.x = b.dir.x;
		b.dir.x = aux;
		aux = dir.y;
		dir.y = b.dir.y;
		b.dir.y = aux;
	}

}