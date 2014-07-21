#include <stdio.h>
#include "lbeTransform.h"

int main (){
	lbeMatrix a;
	lbeLoadIdentity (&a);
	a.m[0][0] = a.m[1][1] = a.m[2][2] = a.m[3][3] = 2;
	//Si esta es la matriz "en papel", la matriz física invaiante, entonces al meterela así,
	//estás metiéndola en "row major", o sea, como si la estuvieses metiendo avanzando el posiciones
	//sucesivas de memoria (iterando el segundo rápido y el primero lento) y dando la matriz por filas.
	//O sea que, si la imprimes usando lbePrintMatrix(), que está compensada para imprimir matrices en 
	//column major, la verás traspuesta. 
	//Con trasponerla antes de imprimirla, lo arreglas.	
	lbeMatrix b = {{{0.2, 0.5, 0.97, 0.33},
			{1.23, 2.32, 3.22, 1.11},
			{3.99, 9.21, 7.22, 2.18},
			{5.22, 3.98, 2.9, 8.72}}};
	
	lbeMatrix c;
	lbeMatrixMultiply (&c, &b, &a);
	lbeTranspose (&c);
	lbePrintMatrix (&c);
}
