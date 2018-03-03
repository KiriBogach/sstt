var font;
var vehicles = [];
var title = [];

var words;
var index = 0;
var size;

function preload() {
  font = loadFont('TF2.ttf');
}

function setup() {
  createCanvas(windowWidth, windowHeight);
  background(51);
  words = split("Welcome to Kiri's WebSite", " ");
  size = windowWidth * 0.06;
  setWord("Click", title, windowWidth * 0.003, windowHeight * 0.1, size);
  setWord("on", title, windowWidth * 0.16, windowHeight * 0.1, size);
  setWord("the", title, windowWidth * 0.26, windowHeight * 0.1, size);
  setWord("screen", title, windowWidth * 0.38, windowHeight * 0.1, size);
  setWord("to", title, windowWidth * 0.6, windowHeight * 0.1, size);
  setWord("continue", title, windowWidth * 0.7, windowHeight * 0.1, size);
  console.log(windowWidth, windowHeight, size);
}

function draw() {
  background(51);
  stroke(255);
  vehicles.forEach(function(v) {
    v.behaviors();
    v.update();
    v.show();
  });
  stroke(255, 0, 0);
  title.forEach(function(v) {
    v.behaviors();
    v.update();
    v.show();
  });

}

function setWord(word, v, x, y, s) {
  var points = font.textToPoints(word, x, y, s, {
    sampleFactor: 0.1
  });

  for (var i = 0; i < points.length; i++) {
    var pt = points[i];
    var vehicle = new Vehicle(pt.x, pt.y);
    v.push(vehicle);
  }
}

function mouseClicked() {
  vehicles = [];
  var w = windowWidth / 2;
  var h = windowHeight / 2;
  setWord(words[index], vehicles, w - (words[index].length * windowWidth * 0.028) , h, size * 1.3);
  index++;
  index %= words.length;
}