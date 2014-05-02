#include <stdio.h>
#include "lbeTransform.h"

int main (){
	lbeMatrix a = {{{1,0,0,0},
			{0,1,0,0},
			{0,0,1,0},
			{0,0,0,1}}};
	
	lbeMatrix b = {{{0.2, 0.5, 0.97, 0.33},
			{1.23, 2.32, 3.22, 1.11},
			{3.99, 9.21, 7.22, 2.18},
			{5.22, 3.98, 2.9, 8.72}}};
	
	lbeMatrix c;
	lbeMatrixMultiply (&c, &b, &a);
	lbePrintMatrix (&c);
}
