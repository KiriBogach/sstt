var angle = 0;
var slider;
var mult = 0.7;

function setup() {
	createCanvas(400, 400);
	createP("Ángulo:");
	slider = createSlider(0, PI, PI / 4, 0.01, PI);
}

function draw() {
	background(51);
	angle = slider.value();
	stroke(255);
	translate(200, height);
	branch(100);

}

function branch(len) {
	var col = map(len, 4, 100, 255, 0);
	stroke(0, col, 0, 200);
	line(0, 0, 0, -len);
	translate(0, -len);
	if (len > 4) {
		push();
		rotate(angle);
		branch(len * mult);
		pop();
		push();
		rotate(-angle);
		branch(len * mult);
		pop();
	}
}