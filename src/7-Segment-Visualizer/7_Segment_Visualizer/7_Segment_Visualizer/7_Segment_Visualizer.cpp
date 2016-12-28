// 7_Segment_Visualizer.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"


#include <iostream>
#include <fstream>
#include <stdint.h>
#include <string>
#include <algorithm>


class Segments {
public:
	static constexpr int segmentWidth = 20;
	static constexpr int innerSideH = 80;
	static constexpr int innerSideV = innerSideH;
	static constexpr int cornerSpace = 3;


	static constexpr int componentWidth() {
		return 2 * segmentWidth + innerSideH;
	}

	static constexpr int componentHeight() {
		return 3 * segmentWidth + 2 * innerSideV;
	}

	friend std::ostream& operator << (std::ostream& lop, const Segments& rop);

};


std::ostream& operator << (std::ostream& lop, const Segments& rop) {
	// ## insert code
	return lop;
}

class Circle {
public:
	static constexpr int leftPadding = 10;
	static constexpr int topPadding = 190;
	static constexpr int radius = 40;
	static constexpr int lineWidth = 2;

	static constexpr int componentWidth() {
		return leftPadding + radius;
	}

	static constexpr int componentHeight() {
		return topPadding + radius;
	}

	friend std::ostream& operator<< (std::ostream& lop, const Circle& rop);

};


std::ostream& operator<< (std::ostream& lop, const Circle& rop) {
	lop << "<path d=\"m " << rop.leftPadding << " " << rop.topPadding << "\" />"; // move to upper right corner of inner "circle container";
	lop << "<circle cx=\"" << (rop.radius / 2) << "\" cy=\"" << (rop.radius / 2) << "\" r=\"" << (rop.radius) << "\" fill=\"black\" />"; // drwaing the circle;
	lop << "<path d=\"m " << -rop.leftPadding << " " << -rop.topPadding << "\" />"; // move to upper right corner of circle;
	return lop;
}


class Frame {
public:

	static constexpr int lineWidth = 2;
	static constexpr int padding = 10;

	static constexpr Segments segments{};
	static constexpr Circle circle{};

	static constexpr int componentWidth() {
		return 2 * padding + segments.componentWidth() + circle.componentWidth();
	}

	static constexpr int componentHeight() {
		return 2 * padding + std::max(segments.componentHeight(), circle.componentHeight());
	}

	static constexpr int frameGoRight() {
		return componentWidth() - 2 * (lineWidth / 2);
	}

	static constexpr int frameGoDown() {
		return componentHeight() - 2 * (lineWidth / 2);
	}

	friend std::ostream& operator<< (std::ostream& lop, const Frame& rop);

};

std::ostream& operator<< (std::ostream& lop, const Frame& rop) {// draw the frame and the 2 components rigth from current cursor position and end at the same curos position
	lop << "<path d=\"m " << (rop.lineWidth / 2) << " " << (rop.lineWidth / 2) << "\" />"; // go south-east to start drawing the frame there;
	lop << "<path d=\"h " << rop.frameGoRight() << " v " << rop.frameGoDown() <<
		" h " << (-rop.frameGoRight()) <<
		" z \" fill=\"transparent\" stoke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";
	// draw the frame;
	lop << "<path d=\"m " << (rop.padding - (rop.lineWidth / 2)) << " " << (rop.padding - (rop.lineWidth / 2)) << "\" />"; // move to upper left corner of segments;
	lop << rop.segments; // draw the segments;
	lop << "<path d=\"m " << rop.segments.componentWidth() << " 0\" />"; // move to upper left corner of circle;
	lop << rop.circle; // draw the circle;
	lop << "<path d=\"m " << (-rop.segments.componentWidth() - rop.padding) << " " << (-rop.padding) << "\" />"; // move to upper left corner of frame;
	return lop;
}

void writefile() {
	std::fstream output;
	output.open("output.svg", std::ios::out);//## change name of file

	constexpr Frame frame;
	std::string header = R"xxx(<?xml version="1.0" standalone="no"?>
				<svg width="
		)xxx";
	header += std::to_string(frame.componentWidth());
	header += R"xxx(
			" height="
		)xxx";
	header += std::to_string(frame.componentHeight());
	header += R"xxx(
			" version="1.1" xmlns="http://www.w3.org/2000/svg">
		)xxx";
	header += R"xxx(
			<path d="M 0 0" />
		)xxx"; // set cursor position at the beginning;

	std::string eof{ "</svg>" };

	output << header;
	output << frame;
	output << eof;
	output.close();
}


int main()
{	
	writefile();
    return 0;
}

