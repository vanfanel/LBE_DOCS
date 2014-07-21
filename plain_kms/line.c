#include <stdio.h>
#include <stdlib.h>

typedef struct tlinea{
	int	x0, y0, x1, y1;
} tlinea;

typedef struct triangulo{
	int	x0, y0, x1, y1, x2, y2;
} triangulo;

void leer_linea(tlinea* linea);
void linea_bressenham (tlinea* linea);
void leer_triangulo(triangulo* t);

int main (int argc, char *argv[]){
	struct triangulo tri;		
	leer_triangulo (&tri);
	printf ("Datos leídos: x0 = %d, y0 = %d, x1 = %d, y1 = %d\n, x2 = %d, y2 = %d\n", tri.x0, tri.y0, tri.x1, tri.y1, tri.x2, tri.y2 );
	//	linea_bressenham (&linea);	
	return 0;
}

void linea_bressenham (tlinea* linea) {
	//MAC: Pendiente eliminar floats en el bucle while.
	int x0 = (*linea).x0;
	int x1 = (*linea).x1;
	int y0 = (*linea).y0;
	int y1 = (*linea).y1;
	
	int deltaX = x1 - x0; 
	int deltaY = y1 - y0;	 
	int i;

	float incError = 0.0f;
	
	//Se usa para ambas variables
	int incremento = 1;
	
	float slope = (float) deltaY / (float) deltaX;  


	if ((slope >= -1.0f) && (slope <= 1.0f)){
		//La coordenada de barrido es X.	
		
		printf ("***Barrido es X \n");
		printf ("Pendiente = %f\n", slope);
		
		if (deltaX > 0)
			incremento = 1;
		else		
			incremento = -1;
		

		while (x0 != x1){
			x0 += incremento;
			incError += slope;

			if (incError >= 0.5f){
				y0 = y0 + incremento;
				incError -= 1.0f;
			}
		}
		printf ("Punto final: %d, %d\n", x0, y0);					

	}
	else {
		//La coordenada de barrido es Y.	
		
		printf ("***Barrido es Y \n");
		
		slope = 1.0f / slope;
		printf ("Pendiente = %f\n", slope);
		
		if (deltaY > 0)
			incremento = 1;
		else		
			incremento = -1;
	
		while (y0 != y1){ 
			y0 += incremento;
			incError += slope;
			if (incError >= 0.5f){
				x0 = x0 + incremento;
				incError -= 1.0f;
			}
		}
		printf ("Punto final: %d, %d\n", x0, y0);					
	}
}

void leer_linea(tlinea* linea){
	printf ("x0 = ");
	scanf ("%d", &(*linea).x0);
	printf ("y0 = ");
	scanf ("%d", &(*linea).y0);
	printf ("x1 = ");
	scanf ("%d", &(*linea).x1);
	printf ("y1 = ");
	scanf ("%d", &(*linea).y1);
}

void leer_triangulo(triangulo* t){
	printf ("x0 = ");
	scanf ("%d", &(*t).x0);
	printf ("y0 = ");
	scanf ("%d", &(*t).y0);
	printf ("x1 = ");
	scanf ("%d", &(*t).x1);
	printf ("y1 = ");
	scanf ("%d", &(*t).y1);
	printf ("x1 = ");
	scanf ("%d", &(*t).x2);
	printf ("y1 = ");
	scanf ("%d", &(*t).y2);
}

void clasificar_triangulo (triangulo* t){
	//Se traza siempre de izquierda a derecha. Así que: 
	//Tipo 1 - Lado base contínuo ; Tipo 2 - Lado base partido	
	//Como los ángulos están limitados a 0-180º, cuando una proporción deltaX/deltaY salga negativa es que
	//deltaX es negativa (decrece la coordenada X en esa línea). El límite de 180º se hace cogiendo siempre
	//el extremo inferior de cada recta, jamás el superior. De ese modo cualquier ángulo de más de 180º
	//queda reducido a uno de 180º, que es lo que nos interesa para medir ángulos sólo usando proporciones
	//deltaX/deltaY. prop2 es la prop de deltas de la recta que une el punto con menor coordenada Y con el 
	//punto que no tiene la menor ni la mayor.	
	//Para tipo 1: -si prop1 positiva (cuadrante 0-90º), para tipo 1: prop2(+) < prop1(+) 
	//	       -si prop1 negativa (cuadrante 90-180º), para tipo 1: prop2(+) > prop1(-) 
	//		|| prop2 (-) > prop1 (-)	
	//Todo lo demás es tipo 2 (lado base, o sea, parte izquierda, partida.)
	//Todo esto se ve mejor con el esquema de "medio asterisco".

}
