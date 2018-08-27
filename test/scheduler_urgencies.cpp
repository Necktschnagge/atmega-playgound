#include <iostream>
//#include <cstdint>

int main(){
	int value {1};
	int count {0};
	while (value < 256){
		std::cout << count << "  " << value << "  \n";
		value = ((value * (256 + 64)) + 255) >> 8;
		++count;
	}
	value = 255;
	count = 0;
	while (value != 1) {
		std::cout << count << "  " << value << "  \n";
		value = value * 4 / 5;
		++count;
	}
	
}
