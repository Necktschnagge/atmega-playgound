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
#include "../../../Fussl-01/Fussl-01/f_ledline.h"

double xxx, yyy;

void setPos(double x, double y) {
	xxx = x;
	yyy = y;
}

inline void chPos(double dx, double dy) {
	setPos(xxx + dx, yyy + dy);
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
		return 2 * segmentWidth + innerSideH; // ###debug 40 + 80 = 120
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

	static constexpr double component_width() { // ##denug 10 + 40 = 50 
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
		return 2 * padding + segments.component_width() + circle.component_width(); //## debug 2 *10 + 120 + 50
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
	static constexpr double textSpace = 30;

	static constexpr Frame frame{};

	std::string labelText;

	LabledFrame(std::string labelText) : labelText(labelText) {}
	
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
	// Label Frame
	lop << R"( <path d=" M )" << xxx+(rop.lineWidth / 2) << " " << yyy+rop.frameToBeginPosition();
	lop << " v " << rop.textSpace;
	lop << " h " << rop.frameGoRight();
	lop << " v " << -rop.textSpace;
	lop << R"(" fill = "transparent" stroke = "black" stroke-width = ")" << rop.lineWidth << "\" />)";
	// Label Text
	lop << R"(	<text y=")" << std::to_string(yyy + rop.frame.component_height() + 20) << R"(" x=")" << xxx + 30;
	lop << R"(" font-family="Verdana" font-size="20">)" << rop.labelText << "</text>)";
	
	return lop;
}



void prepareFile(std::fstream& stream, std::string filename) {
	//static std::fstream output;
	stream.open(filename + ".svg", std::ios::out);//## change name of file
	
	std::string header = R"xxx(<?xml version="1.0" standalone="no"?>)xxx";
	header += R"xxx( <svg width=")xxx";
	header += std::to_string(190 * 11 + 30);
	header += R"xxx(
			" height="
		)xxx";
	header += std::to_string(280 * 30 + 30);
	header += R"xxx(
			" version="1.1" xmlns="http://www.w3.org/2000/svg">
		)xxx";
	stream << header;
}

void finalizeFile(std::fstream& stream) {
	stream.close();
}



void writesvg(/*std::ostream& output, std::string filename, uint8_t signCode*/) {
	std::fstream output;
	prepareFile(output, "output");
	//output.open("out.svg", std::ios::out);//## change name of file
	
	const LabledFrame labFrame {"0x02345"};
	//std::cout << labFrame.component_width() << std::endl << labFrame.component_height();


	std::string header = "";

	//std::string header = R"xxx(<?xml version="1.0" standalone="no"?>)xxx";
	
	std::string eof{ "</svg>" };

	output << header;
	code = 0b01001100; //signCode;
	code = 0;

	//output << R"xxx( <circle cx="205" cy="295" r="5" fill="red" />)xxx";

	setPos(20, 20);
	uint8_t x = led::signCode('a');
	for (uint8_t row = 0; row < 26; ++row) {
		for (uint8_t col = 0; col < 10; ++col) {

			uint8_t index = col + 10 * row;
			char index_c = index;
			code = led::signCode(index);
			std::stringstream ss, ss2;
			ss2 << index_c;
			std::cout << ss2.str();
			//std::cin.get();
			ss << "0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(index);
			std::string label = std::to_string(col + 10 * row) + " : " + ss.str() + " : " +std::string(ss2.str());
			output << LabledFrame{ label };
			chPos(labFrame.component_width() + 10, 0);
		}
		chPos(10 * -1 * (labFrame.component_width() + 10), labFrame.component_height() + 10);

	}
	output << eof;
	finalizeFile(output);
}


int main(){

	writesvg();
	//std::cin.get();
	return 0;
}

