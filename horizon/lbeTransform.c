/*
 * Funciones de transformación (rotación, traslación, escalado) para el proyecto LBE
 *
 */
#include "lbeTransform.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <GLES2/gl2.h>

#define PI 3.1415926535897932384626433832795f

void lbeLoadIdentity (lbeMatrix *resultado){
	float zeros[4][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};	
	memcpy ((*resultado).m, zeros, sizeof (zeros));
	(*resultado).m[0][0] = (*resultado).m[1][1] = (*resultado).m[2][2] = (*resultado).m[3][3] = 1.0f;
}

void lbeMatrixMultiply (lbeMatrix *resultado, lbeMatrix *matrix_a, lbeMatrix *matrix_b){
	//MAC Arreglado para multiplicar matrices que estén en column-major y el resultado
	//sea el correcto (en column-major también), para así no tener que cambiar el órden de 
	//composición de las matrices respecto al álgebra.
	//Esto implica que TODAS mis matrices van a estar en column-major.
	//Es necesario usar una matriz temporal para almacenar el resultado, ya que no debemos alterar
	//"resultado" durante la multiplicación, por si coincide con matrix_a o matrix_b
	//Recorremos la primera matriz por filas y la segunda por columnas: el producto NUNCA cambia. 
	//El tema es que el índice de columnas es el primero y el de filas el segundo, porque estamos en 
	//column-major. Y recorrer una fila es ir cambiando rápidamente de columna. 
	int i,j;
	float accum = 0.0f;
	lbeMatrix res;	
	for (i= 0; i <= 3; i++){
		res.m[i][0] = 0;
		for (j = 0; j <= 3; j++){
			res.m[i][0] += (*matrix_a).m[j][0] * (*matrix_b).m[i][j];
		}	
	}
	
	for (i= 0; i <= 3; i++){
		res.m[i][1] = 0;
		for (j = 0; j <= 3; j++){
			 res.m[i][1] += (*matrix_a).m[j][1] * (*matrix_b).m[i][j];
		}	
	}
	
	for (i= 0; i <= 3; i++){
		res.m[i][2] = 0;
		for (j = 0; j <= 3; j++){
			 res.m[i][2] += (*matrix_a).m[j][2] * (*matrix_b).m[i][j];
		}	
	}
	
	for (i= 0; i <= 3; i++){
		res.m[i][3] = 0;
		for (j = 0; j <= 3; j++){
			res.m[i][3] += (*matrix_a).m[j][3] * (*matrix_b).m[i][j];

		}	
	}
	memcpy ((*resultado).m, res.m, sizeof((*resultado).m));
}

//Multiplica matrix por vector. Arreglado para matriz en column-major.
void lbeMatrixVectorMultiply (lbeVector *resultado, lbeMatrix *mat, lbeVector *vec){
	int i,j;	
	float accum = 0.0f;
	lbeVector res;
	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
			accum = accum + (*mat).m[j][i] * (*vec).v[j];	
		}
		res.v[i] = accum;
		accum = 0.0f;
	}
	memcpy ((*resultado).v, res.v, sizeof((*resultado).v));
}

void lbePrintVector (lbeVector *vec){
	printf ("Vector resultado: |%f, %f, %f, %f|\n",(*vec).v[0],(*vec).v[1],(*vec).v[2],(*vec).v[3]);
}

/* La función de rotación asume la interpretación de OpenGL "clásico": las transformaciones se hacen 
 * de manera "acumulativa sobre sistema local", o sea, según sistema de coordenadas local al objeto o al 
 * sistema de referencia. Esto quiere decir que las sucesivas transformaciones entran postmultiplicando en 
 * el matrix stack.
 * A este nivel, se considera la rotación como RV+, por lo que será a un nivel de abstracción más alto donde 
 * se implemente la inversión de la transformación si se hace sobre sistema de referencia.  
 * Recuerda: por una parte está el cómo se acumulan las transformaciones, y por otra si consideramos
 * que lo que se transforma es el modelo o es el sistema de referencia. Y ambas cosas influyen en
 * el órden de composición de las transformaciones de manera simultánea.
 */

/*Los radianes son una forma ingeniosa de especificar un ángulo: como se trata del arco cuya longitud es igual al
* radio, las circunferencias más pequeñas tienen un tamaño de radián más pequeño y tienen el mismo número de 
* radianes que las grandes. Son un buen sustituto a los grados, que son sólo una división en partes iguales.	
*/

/*Recuerda que se acumulan las sucesivas transformaciones emulando el mismo modo que usa OpenGL clásico:
 * acumulándolas sobre el sistema local al modelo o el sistema de referencia del observador, o sea, que cada
 * nueva matriz entra postmultiplicando en el matrix stack.*/  

void lbeSimpleRotate (lbeMatrix *resultado, float deg, int axis_x, int axis_y, int axis_z){
	//Las matrices están en column-major. Las razones, en la documentación.
	//Para ello, usamos el primer índice como índice de columnas.
	//Meter una fila es cambiar de columna rápidamente. Así que iteramos más rápido en el primer índice 
	//y metemos la matriz por filas.
	int i;
	float rad = 2 * PI / 360 * deg ;

	float c = cos (rad);
	float s = sin (rad);

	lbeMatrix mat;
	
	if (axis_x == 1){
		mat.m[0][0] = 1; 	
		mat.m[0][1] = 0;  	
		mat.m[0][2] = 0; 
		mat.m[0][3] = 0;
		
		mat.m[1][0] = 0; 	
	   	mat.m[1][1] = c; 	
		mat.m[1][2] = s; 	
		mat.m[1][3] = 0;
		
		mat.m[2][0] = 0;	
	   	mat.m[2][1] = -s; 	
		mat.m[2][2] = c; 	
		mat.m[2][3] = 0;
		
		mat.m[3][0] = 0;
	   	mat.m[3][1] = 0;
	   	mat.m[3][2] = 0; 	
		mat.m[3][3] = 1; 	
	}
	
	if (axis_y == 1){
		mat.m[0][0] = c; 	
		mat.m[0][1] = 0;
		mat.m[0][2] = -s;
		mat.m[0][3] = 0;
		
		mat.m[1][0] = 0;
		mat.m[1][1] = 1; 	
		mat.m[1][2] = 0;
		mat.m[1][3] = 0; 	

		mat.m[2][0] = s; 	
		mat.m[2][1] = 0;
		mat.m[2][2] = c; 	
		mat.m[2][3] = 0; 	
		
		mat.m[3][0] = 0;
		mat.m[3][1] = 0;
		mat.m[3][2] = 0; 	
		mat.m[3][3] = 1; 	
	}
	
	
	if (axis_z == 1){
		mat.m[0][0] = c; 	
		mat.m[0][1] = s;
		mat.m[0][2] = 0;
		mat.m[0][3] = 0; 	
		
		mat.m[1][0] = -s; 	
		mat.m[1][1] = c; 	
		mat.m[1][2] = 0;
		mat.m[1][3] = 0; 	

		mat.m[2][0] = 0;
		mat.m[2][1] = 0;
		mat.m[2][2] = 1; 	
		mat.m[2][3] = 0; 	
		
		mat.m[3][0] = 0;
		mat.m[3][1] = 0;
		mat.m[3][2] = 0; 	
		mat.m[3][3] = 1; 	
	}
	//No debemos pasar result como matriz resultado en el producto porque ya se pasa como factor, y 
	//es alterada durante la multiplicación al ser el resultado.Por eso recogemos sobre res y luego hacemos memcp 
	lbeMatrixMultiply(resultado, resultado, &mat);		
}

/*Función de rotación en torno a eje arbitrario*/
/*Se presupone que el eje recibido es un vector unitario: si su módulo no es 1, aun asi sus coordenadas se
interpretan como razones trigonometricas, como cosenos
*/

void lbeRotate (lbeMatrix* resultado, float deg, float u, float v, float w){
	//Column-major, accediendo a posiciones contiguas de memoria, por lo que la matriz va fila a fila.
	int i;
	float rad = 2.0f * PI / 360.0f * deg ;

	float c = cos (rad);
	float s = sin (rad);
	
	float oneminuscos = 1.0f - c;
	float uu = u * u;
	float vv = v * v;
	float ww = w * w;
	float uv = u * v;
	float uw = u * w;	
	float vw = v * w;
	float su = s * u; 
	float sv = s * v;
	float sw = s * w;
	lbeMatrix rmat;

	rmat.m[0][0] = uu * oneminuscos + c;
	rmat.m[0][1] = uv * oneminuscos + sw;
	rmat.m[0][2] = uw * oneminuscos - sv;
	rmat.m[0][3] = 0;
	
	rmat.m[1][0] = uv * oneminuscos - sw;
	rmat.m[1][1] = vv * oneminuscos + c;
	rmat.m[1][2] = vw * oneminuscos + su;
	rmat.m[1][3] = 0;
	
	rmat.m[2][0] = uw * oneminuscos + sv; 	
	rmat.m[2][1] = vw * oneminuscos - su;	
	rmat.m[2][2] = ww * oneminuscos + c; 	
	rmat.m[2][3] = 0;
	
	rmat.m[3][0] = 0;
	rmat.m[3][1] = 0;
	rmat.m[3][2] = 0; 
	rmat.m[3][3] = 1;
	
	//Acumulamos sobre sistema local: cada nueva matriz entra postmultiplicando.	
	//Se respeta el órden normal en álgebra porque la función de producto está compensada para column-major.
	lbeMatrixMultiply (resultado,  resultado, &rmat);
}	

void lbeTranspose (lbeMatrix *mat){
	int i, j;
	lbeMatrix temp;
	for (i = 0; i <=3 ; i++)	
		for (j = 0; j <=3 ; j++)
			temp.m[j][i] = (*mat).m[i][j];
	memcpy ((*mat).m, temp.m, sizeof((*mat).m));	
} 


/*Caso más general de proyección con perspectiva: frustrum de plano cercano no centrado en 0.0.0
* No se realiza cambio de RHS a LHS, así que las coordenadas NDC resultantes (en GLES realmente también se
* esperan coordenadas NDC, de ahí que si no llevas a cabo ninguna proyección, dibujes lo que dibujes sólo
* se va a ver lo que quede dentro del cubo X [1, -1], Y [1, -1], Z [1, -1]) van a estar respecto a un sistema
* tal que Z positivo es hacia el observador, Z negativo es hacia adelante, X positivo es hacia la derecha e
* Y positivo es hacia arriba. 
* Como no se hace este cambio de RHS a LHS, la fila de Z pierde los signos negativos respecto al desarrollo
* de la sección 6. 
*/
void lbeProjection(lbeMatrix *result, float l, float r, float b, float t, float n, float f) {
	
	//Matriz en column-major. Vamos a recorrer la matriz en papel por columnas al introducirla. 
	//Eso implica cambiar de fila rápidamente, así que incrementamos rápido el 2º índice.
	//Además no se pasa de RHS a LHS, por lo que en la fila de obtención de Z, los miembros tienen signo cambiado
	//En esta implementación no se cambia el signo de n y f, por lo que, si se pasan negativos como es de esperar
	//por estar en el eje Z y "por delante del observador", al tratarse de un RHS y no hacerse el cambio a
	//LHS, se mantienen con el signo original. En el desarrollo original, se les anteponía un - para 
	//cambiar el signo a n y f y así pasar de un RHS a un LHS dentro de esta función, de tal manera que, como
	//el observador siempre está en el 0,0,0 y mira hacia Z negativo, la implementación original sólo "funcionaba"
	//con valores de n y f positivos. Pues en esta implementación, no. Esta implementación es coherente y,
	//si lbeTranslate() con tz negativo manda el objeto al fondo, esta función recibe valores negativos de n y f.

	lbeMatrix matProj;		
	float deltaX = r - l;
	float deltaY = t - b;
	float deltaZ = f - n;

	printf ("n = %f	 l = %f r = %f	deltaX = %f\n",n,l,r,deltaX );

	matProj.m[0][0] = -2 * n / deltaX;	
	matProj.m[0][1] = 0;	
	matProj.m[0][2] = 0;	
	matProj.m[0][3] = 0;	
	
	matProj.m[1][0] = 0;	
	matProj.m[1][1] = -2 * n / deltaY;	
	matProj.m[1][2] = 0;	
	matProj.m[1][3] = 0;	
	
	matProj.m[2][0] = (r + l) / deltaX;	
	matProj.m[2][1] = (t + b) / deltaY;	
	matProj.m[2][2] = (f + n) / deltaZ;	
	matProj.m[2][3] = -1;	
	
	matProj.m[3][0] = 0; 	
	matProj.m[3][1] = 0;	
	matProj.m[3][2] = - (2 * f * n) / deltaZ;	
	matProj.m[3][3] = 0;	
	
	lbePrintMatrix(&matProj);
	//MAC Una proyección siempre se hace en último lugar, independientemente del stack de transformaciones.
	//Así que siempre entra premultiplicando.
	lbeMatrixMultiply (result, &matProj, result);
}

void lbeProjectionORIG(lbeMatrix *result, float l, float r, float b, float t, float n, float f) {
	
	//Matriz en column-major. Vamos a recorrer la matriz en papel por columnas al introducirla. 
	//Eso implica cambiar de fila rápidamente, así que incrementamos rápido el 2º índice.
	//En esta implementación sí se pasa de RHS a LHS, por lo que en la fila de obtención de Z, 
	//los miembros tienen el mismo signo que en el desarrollo de la sección 6.
	//Esto crea una incoherencia con lbeTranslate tal como pasaba en esTransform: nosotros trasladamos
	//hacia Z negativo pero luego los planos n y f los especificamos en positivo. Nonsense!
	
	lbeMatrix matProj;		
	float deltaX = r - l;
	float deltaY = t - b;
	float deltaZ = f - n;

	matProj.m[0][0] = 2 * n / deltaX;	
	matProj.m[0][1] = 0;	
	matProj.m[0][2] = 0;	
	matProj.m[0][3] = 0;	
	
	matProj.m[1][0] = 0;	
	matProj.m[1][1] = 2 * n / deltaY;	
	matProj.m[1][2] = 0;	
	matProj.m[1][3] = 0;	
	
	matProj.m[2][0] = (r + l) / deltaX;	
	matProj.m[2][1] = (t + b) / deltaY;	
	matProj.m[2][2] = -(f + n) / deltaZ;	
	matProj.m[2][3] = -1;	
	
	matProj.m[3][0] = 0; 	
	matProj.m[3][1] = 0;	
	matProj.m[3][2] = -(2 * f * n) / deltaZ;	
	matProj.m[3][3] = 0;	
	lbePrintMatrix(&matProj);
	//MAC Una proyección siempre se hace en último lugar, independientemente del stack de transformaciones.
	//Así que siempre entra premultiplicando.
	lbeMatrixMultiply (result, &matProj, result);
}

void lbeOrthoProjection(lbeMatrix *result, float l, float r, float b, float t, float n, float f){
	//Sólo mapeo de coordenadas a intervalo [-1,1]
	//Va en column-major: el primer índice denota la columna. Almacenamos la matriz columna a columna
	//(recorrer una columna es cambiar rápidamente de fila en la matriz en papel, iterando
	//rápidamente en el segundo índice al mismo tiempo).
	//Está cambiado el signo de todos los elementos de la fila de obtención de la nueva Z, 
	//respecto a la matriz en papel, ya que nosotros no cambiamos el signo de Z (nos mantenemos en RHS).
	float deltaX = r - l; 
	float deltaY = t - b;
	float deltaZ = f - n;
	lbeMatrix proj;
	
	proj.m[0][0] = 2 / deltaX;
	proj.m[0][1] = 0;
	proj.m[0][2] = 0;
	proj.m[0][3] = 0;
	
	proj.m[1][0] = 0;
	proj.m[1][1] = 2 / deltaY;
	proj.m[1][2] = 0;
	proj.m[1][3] = 0;
	
	proj.m[2][0] = 0;
	proj.m[2][1] = 0;
	proj.m[2][2] = 2 / deltaZ;
	proj.m[2][3] = 0;
	
	proj.m[3][0] = - ((r + l) / deltaX) ;
	proj.m[3][1] = - ((t + b) / deltaY); 
	proj.m[3][2] =   ((f + n) / deltaZ);
	proj.m[3][3] = 1;

	//No tenemos que cambiar el órden de producto: tenemos el producto adaptado a column-major.
	//Cada nueva transformación entra postmultiplicando: acumulamos sobre el sistema de ref local.
	lbeMatrixMultiply (result, result, &proj);		
}

void lbeTranslate(lbeMatrix *resultado, float tx, float ty, float tz){
	//MAC Va la matriz en papel en column-major, o sea, el primer índice es el de columnas.
	//Iteramos en el segundo índice más deprisa y vamos metiendo la matriz por columnas (lo que equivale
	//a cambiar rápidamente de fila).
	//Y como estamos trasladando respecto al sistema local transformado, tenemos que transformar el
	//vector de rotación antes de sumarlo a la tercera columna de la matriz de entrada.
	
	(*resultado).m[3][0] += tx * (*resultado).m[0][0] + ty * (*resultado).m[1][0] + tz * (*resultado).m[2][0];
	(*resultado).m[3][1] += tx * (*resultado).m[0][1] + ty * (*resultado).m[1][1] + tz * (*resultado).m[2][1];
	(*resultado).m[3][2] += tx * (*resultado).m[0][2] + ty * (*resultado).m[1][2] + tz * (*resultado).m[2][2];
}

void lbePrintMatrix(lbeMatrix *mat) {
	//Adaptada a column-major. Vas a ver la matriz como es en papel, si esta está en column major.
	int i,j;
	printf ("Matriz resultante: \n");
	for (i=0; i<4; i++){
		for (j=0; j<4; j++){
			printf ("%f ", (*mat).m[j][i]);		
		}
		printf ("\n");
	}
}

void lbeCheckGLError (){
	//Función de checkeo de flags de error activos, para ser usada tras llamadas dudosas.
	GLenum err  = glGetError();
 
        while(err!=GL_NO_ERROR) {
                char* error;
                switch(err) {
                        case GL_INVALID_OPERATION:      error="INVALID_OPERATION";      break;
                        case GL_INVALID_ENUM:           error="INVALID_ENUM";           break;
                        case GL_INVALID_VALUE:          error="INVALID_VALUE";          break;
                        case GL_OUT_OF_MEMORY:          error="OUT_OF_MEMORY";          break;
                        case GL_INVALID_FRAMEBUFFER_OPERATION: error="INVALID_FRAMEBUFFER_OPERATION";  break;
                	default: error = "OTHER ERROR"; break;
		}

		printf ("ERROR - error numerado de OpenGL: %s\n", error); 
                err=glGetError();
        }
}
