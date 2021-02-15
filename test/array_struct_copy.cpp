#include <iostream>

struct ArrayContainer {
	using mem_type = int[5];

	mem_type x;
};

int main() {
	ArrayContainer a, b;
	a.x[0] = 1;
	a.x[1] = 2;
	a.x[2] = 3;
	a.x[3] = 4;
	a.x[4] = 5;
	b = a;
	std::cout << b.x[3] << b.x[4];

	return 0;
}