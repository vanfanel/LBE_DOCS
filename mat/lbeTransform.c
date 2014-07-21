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
	int i,j;
	float accum = 0.0f;
	lbeMatrix res;	
	for (i= 0; i <= 3; i++){
		res.m[0][i] = 0;
		for (j = 0; j <= 3; j++){
			res.m[0][i] += (*matrix_b).m[0][j] * (*matrix_a).m[j][i];
		}	
	}
	
	for (i= 0; i <= 3; i++){
		res.m[1][i] = 0;
		for (j = 0; j <= 3; j++){
			 res.m[1][i] += (*matrix_b).m[1][j] * (*matrix_a).m[j][i];
		}	
	}
	
	for (i= 0; i <= 3; i++){
		res.m[2][i] = 0;
		for (j = 0; j <= 3; j++){
			 res.m[2][i] += (*matrix_b).m[2][j] * (*matrix_a).m[j][i];
		}	
	}
	
	for (i= 0; i <= 3; i++){
		res.m[3][i] = 0;
		for (j = 0; j <= 3; j++){
			res.m[3][i] += (*matrix_b).m[3][j] * (*matrix_a).m[j][i];

		}	
	}
	memcpy ((*resultado).m, res.m, sizeof((*resultado).m));
}

//Multiplica matrix por vector.
void lbeMatrixVectorMultiply (lbeVector *resultado, lbeMatrix *mat, lbeVector *vec){
	int i,j;	
	float accum = 0.0f;
	lbeVector res;
	for (i = 0; i < 4; i++){
		for (j = 0; j < 4; j++){
			accum = accum + (*mat).m[i][j] * (*vec).v[j];	
		}
		res.v[i] = accum;
		accum = 0.0f;
	}
	memcpy ((*resultado).v, res.v, sizeof((*resultado).v));
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

/*Los radianes son una forma ingeniosa de especificar un ángulo: como se trata del arco cuya longitud es igual al
* radio, las circunferencias más pequeñas tienen un tamaño de radián más pequeño y tienen el mismo número de 
* radianes que las grandes. Son un buen sustituto a los grados, que son sólo una división en partes iguales.	
*/

/*Recuerda que se acumulan las sucesivas transformaciones emulando el mismo modo que usa OpenGL clásico:
 * acumulándolas sobre el sistema local al modelo o el sistema de referencia del observador, o sea, que cada
 * nueva matriz entra postmultiplicando en el matrix stack.*/  

void lbeSimpleRotate (lbeMatrix *resultado, float deg, int axis_x, int axis_y, int axis_z){
	//Las matrices están en column-major. Las razones, en la documentación.
	//O sea que pillamos la matriz en papel y vamos metiendo columna a columna por filas,
	//y lo hacemos en órden, iterando rápido en el segundo índice y despacio en el primero.	
	int i;
	float rad = 2 * PI / 360 * deg ;

	float c = cos (rad);
	float s = sin (rad);

	lbeMatrix mat;
	
	if (axis_x == 1){
		mat.m[0][0] = 1 ; 	
		mat.m[0][1] = mat.m[0][2] = mat.m[0][3] = 0; 	

	   	mat.m[1][1] = c; 	
	   	mat.m[1][2] = s; 	
	   	mat.m[1][0] = mat.m[1][3] = 0; 	

	   	mat.m[2][1] = -s; 	
	   	mat.m[2][2] = c; 	
	   	mat.m[2][0] = mat.m[2][3] = 0; 	

	   	mat.m[3][0] = mat.m[3][1] = mat.m[3][2] = 0; 	
	   	mat.m[3][3] = 1 ; 	
	}
	
	if (axis_y == 1){
		mat.m[0][0] = c ; 	
		mat.m[0][2] = -s;
		mat.m[0][1] = mat.m[0][3] = 0;

		mat.m[1][1] = 1; 	
		mat.m[1][0] = mat.m[1][2] = mat.m[1][3] = 0; 	

		mat.m[2][0] = s; 	
		mat.m[2][2] = c; 	
		mat.m[2][1] = mat.m[2][3] = 0; 	

		mat.m[3][0] = mat.m[3][1] = mat.m[3][2] = 0; 	
		mat.m[3][3] = 1 ; 	
	}
	
	
	if (axis_z == 1){
		mat.m[0][0] = c; 	
		mat.m[0][1] = s;
		mat.m[0][2] = mat.m[0][3] = 0; 	

		mat.m[1][0] = -s; 	
		mat.m[1][1] = c; 	
		mat.m[1][2] = mat.m[1][3] = 0; 	

		mat.m[2][2] = 1; 	
		mat.m[2][0] = mat.m[2][1] = mat.m[2][3] = 0; 	

		mat.m[3][0] = mat.m[3][1] = mat.m[3][2] = 0; 	
		mat.m[3][3] = 1 ; 	
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
	int i;
	float rad = 2.0f * PI / 360.0f * deg ;

	float c = cos (rad);
	float s = sin (rad);
	
	//printf ("c = %f\n",c );
	//printf ("s = %f\n",s );
	
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

	rmat.m[1][0] = uv * oneminuscos - sw;
	rmat.m[1][1] = vv * oneminuscos + c;
	rmat.m[1][2] = vw * oneminuscos + su;

	rmat.m[2][0] = uw * oneminuscos + sv; 	
	rmat.m[2][1] = vw * oneminuscos - su;	
	rmat.m[2][2] = ww * oneminuscos + c; 	
	
	rmat.m[0][3] = rmat.m[1][3] = rmat.m[2][3] = rmat.m[3][0] = rmat.m[3][1] = rmat.m[3][2] = 0; 
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
	float t11 = (2 * n) / (r - l);
	float t13 = (r + l) / (r - l);
	
	float t22 = (2 * n) / (t - b);
	float t23 = (t + b) / (t - b);

	float t33 =   (f + n) / (f - n);
	float t34 =   (2 * f) / (f - n); 
	
	lbeMatrix res = {{{t11,   0, t13,   0},
			  {  0, t22, t23,   0}, 
			  {  0,   0, t33, t34},
			  {  0,   0,  1,   0}				
	}};	
	lbePrintMatrix(&res);	
	
	memcpy ((*result).m, res.m, sizeof((*result).m));
}

void lbeOrthoProjection(lbeMatrix *result, float l, float r, float b, float t, float n, float f){
	//Sólo mapeo de coordenadas a intervalo [-1,1]
	//Va en column-major: el primer índice denota la columna.
	//Está cambiado el signo de todos los elementos de la fila de obtención de la nueva Z, 
	//respecto a la matriz en papel, ya que nosotros no cambiamos el signo de Z (nos mantenemos en RHS).
	float deltaX = r - l; 
	float deltaY = t - b;
	float deltaZ = f - n;
	lbeMatrix proj;
	
	proj.m[0][0] = 2 / deltaX;
	proj.m[0][1] = proj.m[0][2] = proj.m[0][3] = 0;

	proj.m[1][1] = 2 / deltaY;
	proj.m[1][0] = proj.m[1][2] = proj.m[1][3] = 0;

	proj.m[2][2] = - 2 / deltaZ;
	proj.m[2][0] = proj.m[2][1] = proj.m[2][3] = 0;

	proj.m[3][0] = - (r + l / deltaX) ;
	proj.m[3][1] = - (t + b / deltaY); 
	proj.m[3][2] = - (f + n / deltaZ);
	proj.m[3][3] = 1;

	//No tenemos que cambiar el órden de producto: tenemos el producto adaptado a column-major.
	lbeMatrixMultiply (result, &proj, result);		
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
