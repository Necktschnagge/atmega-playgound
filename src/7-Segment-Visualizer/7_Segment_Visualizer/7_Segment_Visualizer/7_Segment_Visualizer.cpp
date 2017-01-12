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


	/* representing the 7 bar segments of the display*/
class Segments {
public:
		// if no bar is disabled you can see the shape of the bar filled with white color.
		// lineWidth is the width of the black line you only see in this case.
	static constexpr double lineWidth = 2;
		// every bar has the same breadth / width :
	static constexpr double segmentWidth = 20;
		// the two inner rectangulars have the same dimensions.
		// innerSideX determines the size of these rects and impacts the length of the segments around it.
	static constexpr double innerSideH = 80;
		// see at InnerSideH
	static constexpr double innerSideV = innerSideH;
		// in case you don't want the bars to border on each other, cornerSpace makes a little free space between them
	static constexpr double cornerSpace = 2;

		/* return string("black") if isBlack, otherwise string("white" */
	static std::string color_string(uint8_t isBlack) {
		if (isBlack) return std::string("black");
		return std::string("white");
	}

		/* returns the x-dimension of all 7 bars together */
	static constexpr double component_width() {
		return 2 * segmentWidth + innerSideH;
	}

		/* returns the y-dimension of all 7 bars together */
	static constexpr double component_height() {
		return 3 * segmentWidth + 2 * innerSideV;
	}

	friend std::ostream& operator << (std::ostream& lop, const Segments& rop);
};

	/* "draws" (writes) the 7 bars aka "Segments" to given output stream */
std::ostream& operator << (std::ostream& lop, const Segments& rop) {
	// segment 0:
	lop << "<path d=\"M " << xxx + rop.cornerSpace + 2 * (rop.lineWidth / 2) << " " << yyy + (rop.component_height() / 2);
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << -((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " h " << (rop.innerSideH - 2 * rop.cornerSpace);
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << ((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " l " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << ((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " h " << -(rop.innerSideH - 2 * rop.cornerSpace /*- 2 * 2 * (rop.lineWidth / 2)*/);
	lop << " l " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << -((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color_string(code & (1 << 0)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 1:
	lop << "<path d=\"M " << xxx + (rop.lineWidth / 2) << " " << yyy + rop.cornerSpace + 2 * (rop.lineWidth / 2);
	lop << " v " << (rop.component_height()/2) - 2 * rop.cornerSpace - 3.5 * (rop.lineWidth / 2);
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << -((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " v " << (-(rop.innerSideV - 2 * rop.cornerSpace - 0.5 * (rop.lineWidth/2)));
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color_string(code & (1 << 1)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 2:
	lop << "<path d=\"M " << xxx + rop.cornerSpace + 2 * (rop.lineWidth / 2) << " " << yyy + (rop.lineWidth / 2);
	lop << " h " << rop.component_width() - 2 * rop.cornerSpace - 2 * 2 * (rop.lineWidth / 2);
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << (rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " h " << (-(rop.innerSideH - 2 * rop.cornerSpace /*- 2 * (rop.lineWidth/2)*/));
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color_string(code & (1 << 2)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 3:
	lop << "<path d=\"M " << xxx + rop.component_width() - (rop.lineWidth / 2) << " " << yyy + rop.cornerSpace + 2 * (rop.lineWidth / 2);
	lop << " v " << (rop.component_height() / 2) - 2 * rop.cornerSpace - 3.5 * (rop.lineWidth / 2);
	lop << " l " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << -((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " v " << (-(rop.innerSideV - 2 * rop.cornerSpace - 0.5 * (rop.lineWidth / 2)));
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color_string(code & (1 << 3)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 4:
	lop << "<path d=\"M " << xxx + (rop.lineWidth / 2) << " " << yyy + rop.component_height() - rop.cornerSpace - 2 * (rop.lineWidth / 2);
	lop << " v " << -(rop.component_height() / 2) + 2 * rop.cornerSpace + 3.5 * (rop.lineWidth / 2);
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << ((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " v " << (rop.innerSideV - 2 * rop.cornerSpace - 0.5 * (rop.lineWidth / 2));
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << (rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color_string(code & (1 << 4)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 5:
	lop << "<path d=\"M " << xxx + rop.cornerSpace + 2 * (rop.lineWidth / 2) << " " << yyy + rop.component_height() - (rop.lineWidth / 2);
	lop << " h " << rop.component_width() - 2 * rop.cornerSpace - 2 * 2 * (rop.lineWidth / 2);
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " h " << (-(rop.innerSideH - 2 * rop.cornerSpace /*- 2 * (rop.lineWidth/2)*/));
	lop << " l " << (-(rop.segmentWidth - 2 * (rop.lineWidth / 2))) << " " << (rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color_string(code & (1 << 5)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	// segment 6:
	lop << "<path d=\"M " << xxx + rop.component_width() - (rop.lineWidth / 2) << " " << yyy + rop.component_height() - rop.cornerSpace - 2 * (rop.lineWidth / 2);
	lop << " v " << -((rop.component_height() / 2) - 2 * rop.cornerSpace - 3.5 * (rop.lineWidth / 2));
	lop << " l " << -(rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << ((rop.segmentWidth / 2) - 1 * (rop.lineWidth / 2));
	lop << " v " << (rop.innerSideV - 2 * rop.cornerSpace - 0.5 * (rop.lineWidth / 2));
	lop << " l " << (rop.segmentWidth - 2 * (rop.lineWidth / 2)) << " " << (rop.segmentWidth - 2 * (rop.lineWidth / 2));
	lop << " z \" fill=\"" << rop.color_string(code & (1 << 6)) << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />";

	return lop;
}

	/* representation of the dot */
class Circle {
public:
		// white space on the left
	static constexpr double leftPadding = 10;
		// white space above
	static constexpr double topPadding = 190;
		// radius of circle
	static constexpr double radius = 20;
		// border line width if the circle bit is not set
	static constexpr double lineWidth = 2;

	static std::string dot_text() {
		if (code & (1 << 7)) {
			return std::string("black");
		}
		else {
			return std::string("white");
		}
	}

	static constexpr double component_width() {
		return leftPadding + 2*radius;
	}

	static constexpr double component_height() {
		return topPadding + 2*radius;
	}

	friend std::ostream& operator<< (std::ostream& lop, const Circle& rop);

};


std::ostream& operator<< (std::ostream& lop, const Circle& rop) {
	lop << "<circle cx=\"" << (xxx + rop.leftPadding + rop.radius) << "\" cy=\"" << (yyy + rop.topPadding + rop.radius) << "\" r=\"" << (rop.radius);
	lop << "\" fill=\"" << rop.dot_text() << "\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />"; // drwaing the circle;
	return lop;
}

	/* representation of a whole 7-Segment LED with dot */
class Frame {
public:
		// line width of the frame around the 7seg display
	static constexpr double lineWidth = 2;
		// white space (on every side equal) to the inner components
	static constexpr double padding = 10;
		
	static constexpr Segments segments{};
	static constexpr Circle circle{};

	static constexpr double component_width() {
		return 2 * padding + segments.component_width() + circle.component_width();
	}

	static constexpr double component_height() {
		return 2 * padding + std::max(segments.component_height(), circle.component_height());
	}

	inline static constexpr double frameGoRight() {
		return component_width() - 2 * (lineWidth / 2);
	}

	inline static constexpr double frameGoDown() {
		return component_height() - 2 * (lineWidth / 2);
	}

	friend std::ostream& operator<< (std::ostream& lop, const Frame& rop);

};

	/* draw the frame and the 2 components inside. Cursor Positionn is the left upper corner of drawn stuff and will be the same before and after the function was called */
std::ostream& operator<< (std::ostream& lop, const Frame& rop) {
	lop << "<path d=\"M " << xxx+(rop.lineWidth / 2) << " " << yyy+(rop.lineWidth / 2); // go south-east to start drawing the frame there;
	lop << " h " << rop.frameGoRight() << " v " << rop.frameGoDown() <<
		" h " << (-rop.frameGoRight()) <<
		" z \" fill=\"transparent\" stroke=\"black\" stroke-width=\"" << rop.lineWidth << "\" />"; // draw the frame;

	setPos(xxx + rop.padding, yyy + rop.padding);
	lop << rop.segments; // draw the segments;
	setPos(xxx + rop.segments.component_width(), yyy);
	lop << rop.circle; // draw the circle;
	setPos(xxx - rop.padding - rop.segments.component_width(), yyy - rop.padding);

	return lop;
}

class LabledFrame {
public:
	// line width of the frame around the label
	static constexpr double lineWidth = 2;
	// white space neede for text
	static constexpr double textSpace = 10;

	static constexpr Frame frame{};

	static constexpr double component_width() {
		return frame.component_width();
	}

	static constexpr double component_height() {
		return frame.component_height() + textSpace;
	}

	inline static constexpr double frameGoRight() {
		return component_width() - 2 * (lineWidth / 2);
	}

	inline static constexpr double frameToBeginPosition() {
		return frame.component_height() - (frame.lineWidth / 2);
	}

	friend std::ostream& operator<< (std::ostream& lop, const LabledFrame& rop);

};

std::ostream& operator<< (std::ostream& lop, const LabledFrame& rop) {
	lop << rop.frame;
	lop << R"( <path d=" M )" << (rop.lineWidth / 2) << " " << rop.frameToBeginPosition();
	lop << " v " << rop.textSpace;
	lop << " h " << rop.frameGoRight();
	lop << " v " << -rop.textSpace;
	lop << R"(" fill = "transparent" stroke = "black" stroke - width = ")" << rop.lineWidth << "\" />)";

	return lop;
}



std::fstream& prepareFile(std::string filename) {
	static std::fstream output;
	output.open(filename + ".svg", std::ios::out);//## change name of file
	
	std::string header = R"xxx(<?xml version="1.0" standalone="no"?>)xxx";
	output << header;
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
	header += std::to_string(frame.component_width());
	header += R"xxx(
			" height="
		)xxx";
	header += std::to_string(frame.component_height());
	header += R"xxx(
			" version="1.1" xmlns="http://www.w3.org/2000/svg">
		)xxx";
	std::string eof{ "</svg>" };

	output << header;
	code = 0b01001100; //signCode;
	setPos(0, 0);
	output << frame;
	output << R"(	<text y=")" << std::to_string(frame.component_height()+ 30 ) << R"(" x="30" font-family="Verdana" font-size="20"> 0x01 </text>)";
	output << eof;
	output.close();
}


int main(){

	writesvg();
	return 0;
}

