// 7_Segment_Visualizer.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <string>
#include <algorithm>

double xxx, yyy;

void setPos(double x, double y) {
	xxx = x;
	yyy = y;
}

uint8_t code;

class Segments {
public:
	static constexpr double lineWidth = 2;
	static constexpr double segmentWidth = 20;
	static constexpr double innerSideH = 80;
	static constexpr double innerSideV = innerSideH;
	static constexpr double cornerSpace = 2;

	static std::string color(uint8_t isBlack) {
		if (isBlack) return std::string("black");
		return std::string("white");
	}

	static constexpr double componentWidth() {
		return 2 * segmentWidth + innerSideH;
	}

	static constexpr double componentHeight() {
		return 3 * segmentWidth + 2 * innerSideV;
	}

	friend std::ostream& operator << (std::ostream& lop, const Segments& rop);

};


std::ostream& operator << (std::ostream& lop, const Segments& rop) {
	// ## insert code

	// segment 0:

	lop << "<path d=\"M " << xxx + rop.cornerSpace + 2 * (rop.lineWidth / 2) << " " << yyy + (rop.componentHeight() / 2);
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << -((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " h " << (rop.innerSideH - 2 * rop.cornerSpace);
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << ((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " l " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << ((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " h " << -(rop.innerSideH - 2 * rop.cornerSpace /*- 2 * 2 * (rop.lineWidth / 2)*/);
	lop << " l " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << -((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color(code & (1 << 0)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 1:

	lop << "<path d=\"M " << xxx + (rop.lineWidth / 2) << " " << yyy + rop.cornerSpace + 2 * (rop.lineWidth / 2);
	lop << " v " << (rop.componentHeight()/2) - 2 * rop.cornerSpace - 3.5 * (rop.lineWidth / 2);
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << -((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " v " << (-(rop.innerSideV - 2 * rop.cornerSpace - 0.5 * (rop.lineWidth/2)));
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color(code & (1 << 1)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";


	// segment 2:
	lop << "<path d=\"M " << xxx + rop.cornerSpace + 2 * (rop.lineWidth / 2) << " " << yyy + (rop.lineWidth / 2);
	lop << " h " << rop.componentWidth() - 2 * rop.cornerSpace - 2 * 2 * (rop.lineWidth / 2);
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << (rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " h " << (-(rop.innerSideH - 2 * rop.cornerSpace /*- 2 * (rop.lineWidth/2)*/));
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color(code & (1 << 2)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 3:

	lop << "<path d=\"M " << xxx + rop.componentWidth() - (rop.lineWidth / 2) << " " << yyy + rop.cornerSpace + 2 * (rop.lineWidth / 2);
	lop << " v " << (rop.componentHeight() / 2) - 2 * rop.cornerSpace - 3.5 * (rop.lineWidth / 2);
	lop << " l " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << -((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " v " << (-(rop.innerSideV - 2 * rop.cornerSpace - 0.5 * (rop.lineWidth / 2)));
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color(code & (1 << 3)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 4:

	lop << "<path d=\"M " << xxx + (rop.lineWidth / 2) << " " << yyy + rop.componentHeight() - rop.cornerSpace - 2 * (rop.lineWidth / 2);
	lop << " v " << -(rop.componentHeight() / 2) + 2 * rop.cornerSpace + 3.5 * (rop.lineWidth / 2);
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << ((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " v " << (rop.innerSideV - 2 * rop.cornerSpace - 0.5 * (rop.lineWidth / 2));
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << (rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color(code & (1 << 4)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 5:
	lop << "<path d=\"M " << xxx + rop.cornerSpace + 2 * (rop.lineWidth / 2) << " " << yyy + rop.componentHeight() - (rop.lineWidth / 2);
	lop << " h " << rop.componentWidth() - 2 * rop.cornerSpace - 2 * 2 * (rop.lineWidth / 2);
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " h " << (-(rop.innerSideH - 2 * rop.cornerSpace /*- 2 * (rop.lineWidth/2)*/));
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << (rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color(code & (1 << 5)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 6:

	lop << "<path d=\"M " << xxx + rop.componentWidth() - (rop.lineWidth / 2) << " " << yyy + rop.componentHeight() - rop.cornerSpace - 2 * (rop.lineWidth / 2);
	lop << " v " << -((rop.componentHeight() / 2) - 2 * rop.cornerSpace - 3.5 * (rop.lineWidth / 2));
	lop << " l " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << ((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " v " << (rop.innerSideV - 2 * rop.cornerSpace - 0.5 * (rop.lineWidth / 2));
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << (rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color(code & (1 << 6)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";


	return lop;
}

class Circle {
public:
	static constexpr double leftPadding = 10;
	static constexpr double topPadding = 190;
	static constexpr double radius = 20;
	static constexpr double lineWidth = 2;

	static std::string dotText() {
		if (code & (1 << 7)) {
			return std::string("black");
		}
		else {
			return std::string("white");
		}
	}

	static constexpr double componentWidth() {
		return leftPadding + 2*radius;
	}

	static constexpr double componentHeight() {
		return topPadding + 2*radius;
	}

	friend std::ostream& operator<< (std::ostream& lop, const Circle& rop);

};


std::ostream& operator<< (std::ostream& lop, const Circle& rop) {
	lop << "<circle cx=\"" << (xxx + rop.leftPadding + rop.radius) << "\" cy=\"" << (yyy + rop.topPadding + rop.radius) << "\" r=\"" << (rop.radius);
	lop << "\" fill=\"" << rop.dotText() << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />"; // drwaing the circle;
	return lop;
}


class Frame {
public:

	static constexpr double lineWidth = 2;
	static constexpr double padding = 10;

	static constexpr Segments segments{};
	static constexpr Circle circle{};

	static constexpr double componentWidth() {
		return 2 * padding + segments.componentWidth() + circle.componentWidth();
	}

	static constexpr double componentHeight() {
		return 2 * padding + std::max(segments.componentHeight(), circle.componentHeight());
	}

	static constexpr double frameGoRight() {
		return componentWidth() - 2 * (lineWidth / 2);
	}

	static constexpr double frameGoDown() {
		return componentHeight() - 2 * (lineWidth / 2);
	}

	friend std::ostream& operator<< (std::ostream& lop, const Frame& rop);

};

std::ostream& operator<< (std::ostream& lop, const Frame& rop) {// draw the frame and the 2 components rigth from current cursor position and end at the same curos position
	lop << "<path d=\"M " << xxx+(rop.lineWidth / 2) << " " << yyy+(rop.lineWidth / 2); // go south-east to start drawing the frame there;
	lop << " h " << rop.frameGoRight() << " v " << rop.frameGoDown() <<
		" h " << (-rop.frameGoRight()) <<
		" z \" fill=\"transparent\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />"; // draw the frame;

	setPos(xxx + rop.padding, yyy + rop.padding);
	lop << rop.segments; // draw the segments;
	setPos(xxx + rop.segments.componentWidth(), yyy);
	lop << rop.circle; // draw the circle;
	setPos(xxx - rop.padding - rop.segments.componentWidth(), yyy - rop.padding);

	return lop;
}

std::fstream& prepareFile(std::string filename) {
	static std::fstream output;
	output.open(filename + ".svg", std::ios::out);//## change name of file
	
	std::string header = R"xxx(<?xml version="1.0" standalone="no"?>)xxx";
	return output;
}

void finalizeFile(std::fstream& stream) {
	stream.close();
}



void writesvg(/*std::ostream& output, std::string filename, uint8_t signCode*/) {
	std::fstream output;
	output.open("out.svg", std::ios::out);//## change name of file
	
	constexpr Frame frame;
	

	//std::string header;

	std::string header = R"xxx(<?xml version="1.0" standalone="no"?>)xxx";
	header += R"xxx( <svg width=")xxx";
	header += std::to_string(frame.componentWidth());
	header += R"xxx(
			" height="
		)xxx";
	header += std::to_string(frame.componentHeight());
	header += R"xxx(
			" version="1.1" xmlns="http://www.w3.org/2000/svg">
		)xxx";
	std::string eof{ "</svg>" };

	output << header;
	code = 0b01001100; //signCode;
	setPos(0, 0);
	output << frame;
	output << eof;
	output.close();
}


int main(){

	writesvg();
	//std::fstream& stream = prepareFile("output");
	/*
	for (uint16_t i = 0; i < 256; ++i) {
		std::stringstream mystream;
		mystream << std::setfill('0') << std::setw(2) << std::hex << i;
		std::string result(mystream.str());
		//writesvg(std::to_string(i), i);####
	}
	*/
	return 0;
}

