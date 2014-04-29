/*
 * Funciones de transformación (rotación, traslación, escalado) para el proyecto LBE
 *
 */
#include "lbeTransform.h"

void lbeLoadIdentity (lbeMatrix *resultado){
	 lbeMatrix res = {{{1, 0, 0, 0},
		           {0, 1, 0, 0},
		           {0, 0, 1, 0},
		           {0, 0, 0, 1}}};
}

void lbeMatrixMultiply (lbeMatrix *resultado, lbeMatrix *matrix_a, lbeMatrix *matrix_b){
	int i,j,accum;
	accum = 0;

	//La idea es que dejamos fija una fila de la primera matriz y vamos avanzando en la segunda por columnas.
	//En bloques porque en uno solo habría que introducir un iterador más y eso complica la lectura.
	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
				accum = accum + (*matrix_a).m[0][j] * (*matrix_b).m[j][i];
		}
		(*resultado).m[0][i] = accum; 
		accum = 0;
	}
	
	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
				accum = accum + (*matrix_a).m[1][j] * (*matrix_b).m[j][i];
		}
		(*resultado).m[1][i] = accum; 
		accum = 0;
	}
	
	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
				accum = accum + (*matrix_a).m[2][j] * (*matrix_b).m[j][i];
		}
		(*resultado).m[2][i] = accum; 
		accum = 0;
	}

	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
				accum = accum + (*matrix_a).m[3][j] * (*matrix_b).m[j][i];
		}
		(*resultado).m[3][i] = accum; 
		accum = 0;
	}
}

/* La función de rotación asume la interpretación de OpenGL "clásico": las transformaciones se hacen 
 * de manera "acumulativa sobre sistema local", o sea, según sistema de coordenadas local al objeto o al 
 * sistema de referencia. Esto quiere decir que las sucesivas transformaciones entran postmultiplicando en 
 * el matrix stack.
 * A este nivel, se considera la rotación como RV+, por lo que será a un nivel de abstracción más alto donde 
 * se implemente la inversión de la transformación si se hace sobre sistema de referencia.  
 * Recuerda: por una parte está el cómo se acumulan las transformaciones, y por otra si consideramos
 * que se transforma el modelo o el sistema de referencia.
 */

//void lbeRotate (float **mat, int axis_x, int axis_y, int axis_z, float rad){
/*Los radianes son una forma ingeniosa de especificar un ángulo: como se trata del arco cuya longitud es igual al
* radio, las circunferencias más pequeñas tienen un tamaño de radián más pequeño y tienen el mismo número de 
* radianes que las grandes. Son un buen sustituto a los grados, que son sólo una división en partes iguales.	
*/

/*Te darás cuenta de que este layout no se corresponde con la colocación que esperarías para operar directamente
* con esta matriz (en postmul o premul): es que eso es a parte, lo hace OpenGL internamente en base a cómo
* traen los shaders definido el producto de matriz con vector. NO ESPERES una relación inmediata entre el layout
* que espera OpenGL y cómo han de estar los elementos para operar con la matriz: voy a colocar el vector de 
* traslación en lo que parece ser la última fila (en realidad no hay ninguna fila porque OpenGL espera la matriz
* en una formación unidimensional), aunque te extrañe porque debería estar en la última columna si pones los 
* vectores unitarios por filas, pero por eso, porque es un layout para OpenGL y no se corresponde con el layout
* para operar con la matriz directamente: no busques tal relación porque sólo encontrarás confusión.
*/

/*De momento, aceptamos que es posible recibir más de una rotación en torno a eje (por ejemplo con 1,0,1) pero
 * como es distinto rotar en torno a X y luego en torno a Z que al revés (lo mires como lo mires, debido a
 * que el producto de matrices no es conmutativo), fijamos que el órden en que que harán rotaciones sucesivas 
 * en torno a distintos ejes siempre será: primero en torno a X, después respecto a Y y por último respecto a Z.
 * Queda pendiente implementar un órden concreto, usando valores distintos de 1 y de 0.
*/

/*Recuerda que se acumulan las sucesivas transformaciones emulando el mismo modo que usa OpenGL clásico:
 * acumulándolas sobre el sistema local al modelo o el sistema de referencia del observador, o sea, que cada
 * nueva matriz entra postmultiplicando en el matrix stack.*/  
/*	int i;
	int rot[] = {axis_x, axis_y, axis_z };

	float c = cos (rad);
	float s = sin (rad);

	//Nos imaginamos que cada linea va de arriba a abajo: tres columnas de 16 elementos cada.

	       {1, 0, 0, 0, 
		0, c,-s, 0, 
		0, s, c, 0, 
		0, 0, 0, 1},

	       {c, 0, s, 0, 
	        0, 1, 0, 0, 
	       -s, 0, c, 0, 
		0, 0, 0, 1},

	       {c,-s, 0, 0, 
		s, c, 0, 0, 
		0, 0, 1, 0, 
		0, 0, 0, 1}
	}	

	if (axis_x == 1) 	
		*mat = *mat 
	if (axis_y == 1) 	
	
	if (axis_z == 1) 	

}*/

lbeProjection() {

}



