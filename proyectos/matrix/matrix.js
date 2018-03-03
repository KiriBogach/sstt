var texto;
var symbolSize = 14;
var streams = [];
var fraseSecreta = [];
var input, button;
var music;

function preload() {
	soundFormats('mp3');
	music = loadSound("matrix.mp3");
}

function setup() {
	createCanvas(window.innerWidth, window.innerHeight);
	background(0);
	textSize(symbolSize);

	var x = 0;
	for (var i = 0; i < width / symbolSize; i++) {
		var stream = new Stream();
		stream.generateSymbols(x, random(-1000, 0));
		streams.push(stream);
		x += symbolSize;
	}

	music.play();
	input = createInput();
 	input.position(window.innerWidth * 0.01, window.innerHeight * 0.01);
 	button = createButton('submit');
  	button.position(input.x + input.width, window.innerHeight * 0.01);
  	button.mousePressed(update);
}

function draw() {
	background(0, 160);
	streams.forEach(function(stream) {
		stream.render();
	});
	
	fraseSecreta.forEach(function(stream) {
		stream.render();
	});
	
}

function update() {
	fraseSecreta = [];
	texto = input.value();
	for (var i = 0; i < 50; i++) {
		var frase = new Message(texto);
		frase.generateSymbols(random(-width, width), random(-1000, 0));
		fraseSecreta.push(frase);
	}
}

function Symbol(x, y, speed, first) {
	this.x = x;
	this.y = y;
	this.speed = speed;
	this.value;
	this.velocidadCambio = round(random(2, 20));
	this.first = first;

	this.setToRandomSymbol = function() {
		if (frameCount % this.velocidadCambio == 0) {
			this.value = String.fromCharCode(
			0x30A0 + round(random(0, 96))
			);
		}
	}

	this.setSymbol = function(letra) {
		this.value = letra;
	}

	this.gravity = function() {
		this.y = (this.y >= height) ? 0 : this.y += this.speed;
	}	
}

function Stream() {
	this.symbols = [];
	this.totalSymbols = round(random(5, 35));
	this.speed = random(5, 15);

	this.generateSymbols = function(x, y) {
		var first = round(random(0, 4)) == 1;
		for (var i = 0; i < this.totalSymbols; i++) {
			symbol = new Symbol(x, y, this.speed, first);
			symbol.setToRandomSymbol();
			this.symbols.push(symbol);
			y -= symbolSize;
			first = false;
		}
	}

	this.render = function() {
		this.symbols.forEach(function(symbol) {
			if (symbol.first) {
				fill(180, 255, 180);
			}
			else fill(0, 255, 70);
			text(symbol.value, symbol.x, symbol.y);
			symbol.gravity();
			symbol.setToRandomSymbol();
		});
	}
}

function Message(str) {
	this.symbols = [];
	this.mensaje = str.split('').reverse().join('');
	this.long = this.mensaje.length;
	this.speed = random(6, 16);

	this.generateSymbols = function(x, y) {
		var first = round(random(0, 4)) == 1;
		for (var i = 0; i < this.long; i++) {
			symbol = new Symbol(x, y, this.speed, first);
			symbol.setSymbol(this.mensaje[i]);
			this.symbols.push(symbol);
			y -= symbolSize;
			first = false;
		}
	}

	this.render = function() {
		textSize(18);
		this.symbols.forEach(function(symbol) {
			if (symbol.first) {
				fill(180, 255, 180);
			}
			else fill(220, 60, 40);
			text(symbol.value, symbol.x, symbol.y);
			symbol.gravity();
		});
		textSize(symbolSize);
	}
}