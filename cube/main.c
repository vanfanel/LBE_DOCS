#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "context.h"
#include "lbeTransform.h"
#include "teclado.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

GLenum err;
extern float rAngle, rSpeed, rSign;
extern int exit_condition;

//Aquí vamos a guardar ambos shaders una vez compilados: le hacemos un attach a ambos a este objeto.
GLuint programObject;

//Las matrices que vamos a ir usando y sus object handles (los GLints)
lbeMatrix mvp;
GLint mvpOBJ;
lbeMatrix projection;

GLbyte vertexShaderSrc[] = 
	"uniform mat4 modelviewprojection;	\n"
	"attribute vec4 vertexPosition;		\n"
	"attribute vec4 vertexColor;		\n"
	"varying vec4 vyVertexColor;		\n"
	"void main () {				\n"
	//"	vyVertexColor = vec4 (1.0, 0.0, 0.0, 1.0);		\n"
	"	vyVertexColor = vertexColor;				\n"
	"	gl_Position = modelviewprojection * vertexPosition;	\n"
	//"	gl_Position = vertexPosition;				\n"
	"}								\n"
;

GLbyte fragmentShaderSrc[] = 
	"precision mediump float;			 \n"
	"varying vec4 vyVertexColor;			 \n"
	"void main (){				 	 \n"
	"	 gl_FragColor = vyVertexColor;		 \n"
	//"	 gl_FragColor = vec4 (1.0, 0.0, 0.0, 1.0);	 \n"
	"}					 	 \n"
;

/*static const float vertices[] = {0.0f, 0.5f, 0.0f,
		   -0.5f, -0.5f, 0.0f,
		    0.5f, -0.5f, 0.0f};
	*/

static const float vertices[] = {
	// front
	-1.0f, -1.0f, 1.0f, // point blue
	+1.0f, -1.0f, 1.0f, // point magenta
	-1.0f, +1.0f, 1.0f, // point cyan
	+1.0f, +1.0f, 1.0f, // point white*/
	// back
	+1.0f, -1.0f, -1.0f, // point red
	-1.0f, -1.0f, -1.0f, // point black
	+1.0f, +1.0f, -1.0f, // point yellow
	-1.0f, +1.0f, -1.0f, // point green
	// right
	+1.0f, -1.0f, +1.0f, // point magenta
	+1.0f, -1.0f, -1.0f, // point red
	+1.0f, +1.0f, +1.0f, // point white
	+1.0f, +1.0f, -1.0f, // point yellow
	// left
	-1.0f, -1.0f, -1.0f, // point black
	-1.0f, -1.0f, +1.0f, // point blue
	-1.0f, +1.0f, -1.0f, // point green
	-1.0f, +1.0f, +1.0f, // point cyan*/
	// top
	-2.0f, +2.0f, +2.0f, // point cyan
	+2.0f, +2.0f, +2.0f, // point white
	-2.0f, +2.0f, -2.0f, // point green
	+2.0f, +2.0f, -2.0f, // point yellow
	// bottom
	-2.0f, -2.0f, -2.0f, // point black
	+2.0f, -2.0f, -2.0f, // point red
	-2.0f, -2.0f, +2.0f, // point blue
	+2.0f, -2.0f, +2.0f // point magenta

};

static const GLfloat colors[] = {
	// front
	1.0f, 0.0f, 0.0f, // blue
	1.0f, 0.0f, 0.0f, // magenta
	1.0f, 0.0f, 0.0f, // cyan
	1.0f, 0.0f, 0.0f, // white
	// back
	0.5f, 0.0f, 0.0f, // red
	0.5f, 0.0f, 0.0f, // black
	0.5f, 0.0f, 0.0f, // yellow
	0.5f, 0.0f, 0.0f, // green
	// right
	0.0f, 0.0f, 1.0f, // magenta
	0.0f, 0.0f, 1.0f, // red
	0.0f, 0.0f, 1.0f, // white
	0.0f, 0.0f, 1.0f, // yellow
	// left
	1.0f, 1.0f, 1.0f, // black
	1.0f, 1.0f, 1.0f, // blue
	1.0f, 1.0f, 1.0f, // green
	1.0f, 1.0f, 1.0f, // cyan
	// top
	0.0f, 0.5f, 0.5f, // cyan
	0.5f, 0.5f, 0.5f, // white
	0.0f, 0.5f, 0.0f, // green
	0.5f, 0.5f, 0.0f, // yellow
	// bottom
	0.0f, 0.0f, 0.0f, // black
	0.5f, 0.0f, 0.0f, // red
	0.0f, 0.0f, 0.5f, // blue
	0.5f, 0.0f, 0.5f // magenta
};


static int init_gl(void)
{
	GLint attPositions;
	GLint attColors;
	
	//Establecemos el viewport
	printf ("Usando modo de vídeo %d x %d\n", eglInfo.width, eglInfo.height);
	glViewport (0, 0, eglInfo.width, eglInfo.height);
	
	//MAC La superficie de proyección es de ratio 1:1. La pantalla en cambio es 16(width):9(height), 
	//por lo que las cosas aparecen demasiado anchas. El ratio para compensar es height/width	
	//ya que la altura es menor y nos interesa un ratio < 0.
	float ratio = (float)eglInfo.height / (float)eglInfo.width;
	printf ("Phisical ratio W/H: %f\n", ratio);
	
	//Cogemos el número de atributo de vertexPosition en el código del vertex shader. 
	//Alternativamente, podríamos especificar el nuestro con glBindAttribLocation(), pero 
	//antes de linkar el programa.
	attPositions = glGetAttribLocation (programObject, "vertexPosition"); 
	attColors = glGetAttribLocation (programObject, "vertexColor");

	//Activamos cada atributo para que pueda ser usado desde el vertex shader
	glEnableVertexAttribArray(attPositions);	
	glEnableVertexAttribArray(attColors);	

	//Subimos los datos al buffer de atributos de vértices, que previamente creamos
	GLuint vertexBuffer;
	glGenBuffers (1, &vertexBuffer);
	
	glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);

	uintptr_t positionsOffset = 0;
	uintptr_t colorsOffset = (sizeof(vertices));
	
	glBufferData (GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors), NULL, GL_STATIC_DRAW);
	glBufferSubData (GL_ARRAY_BUFFER, positionsOffset, sizeof(vertices), vertices);
	glBufferSubData (GL_ARRAY_BUFFER, colorsOffset, sizeof(colors), colors);
	
	//Aquí lo que hacemos es enlazar los atributos en el shader con las direcciones de cada uno de los 
	//arrays que tenemos ya en el buffer: posición, color, normal
	//RECUERDA: UN BUFFER OBJECT, HASTA TRES ATRIBUTOS POR CADA VÉRTICE QUE GUARDAMOS EN ÉL
	glVertexAttribPointer(attPositions, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)positionsOffset);
	glVertexAttribPointer(attColors, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)colorsOffset);

	//Deshacemos el binding para no alterar los datos del buffer sin querer.
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//Subimos a GL la matriz de transformación + proyeccción. La secuencia es parecida.
	//Pendiente agrupar todas las variables que son operacionales a este nivel en una estructura.
	
	mvpOBJ = glGetUniformLocation (programObject, "modelviewprojection");	

	//Empezamos colocando como matriz modelviewprojection la identidad.
	lbeLoadIdentity (&mvp);	
	glUniformMatrix4fv(mvpOBJ, 1, GL_FALSE, &mvp.m[0][0]);
	
	//Vamos a animar un poco las cosas, a base de ir actualizando el uniform correspondiente a 
	//la matriz mvp, a través del objeto mvpOBJ(GLint), usando la función glUniformMatrix4fv()
	int loops = 0;
	int exit_condition = 0;


	lbeLoadIdentity (&projection);
	
	//Si un objeto aparece más pequeño de lo que esperas, probablemente sea porque el plano de 
	//proyección es demasiado grande (en relación al tamaño del objeto).
	lbeProjection (&projection, -2.0f, 2.0f, -2.0f*ratio, 2.0f*ratio, -2.0f, -10.0f);
	
	/*NO BORRAR ESTE COMENTARIO. 
	* No puedes acumular las sucesivas rotaciones sobre la matriz MVP sin resetearla, porque 
	* también acumularías la proyección y a dos o tres veces que la apliques, algunas coordenadas
	* serán tan pequeñas que no se verá, o quedarán fuera del frustum por salirse la coordenada 
	* final del intervalo [-1,1], o cosas por el estilo. Así que resetea (carga la identidad) la 
	* MVP en cada iteración y, en lugar de rotar un mismo ángulo una y otra vez, rota con un 
	*ángulo cada vez mayor.
	*/
	float rAngle = 0.0f;
	//Cada vez que se recorra esta distancia, habrá que meter una nueva línea de horizonte.
	float tDistance = 0.0f;
	float rTranslate = 0.0f;

	glEnable(GL_CULL_FACE);
	//glEnable(GL_DEPTH_TEST);

	return 0;
}

static void draw_cube(uint32_t i)
{	
	//glDrawArrays(GL_TRIANGLES, 0, 3);
	//Pasamos a dibujar un strip (serie de triángulos conectados) para formar la cara. Nos permite reutilizar
	//los vértices para el segundo triángulo del strip, de manera que con una llamada y cuatro vértices
	//dibujamos un cuadrado.	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
	//printf ("pintado...\n");	
}

static void flip_page(){
	eglSwapBuffers(eglInfo.display, eglInfo.surface);
	//Sólo para el contexto DRM/KMS
	DRM_PageFlip();	

}

GLuint CompileShader (GLuint shaderType, const char *shaderSrc){
	//Función que recibe el tipo de un shader (un define de GL que indica si es un GL_VERTEX_SHADER o 
	//un GL_FRAGMENT_SHADER) y el texto del programa del shader, y se encarga de crear el objeto shader,
	//compilar el programa del shader y comprobar si se ha compilado bien. 
	//Devuelve el objeto compilado como un GLuint.  
	GLuint shader;
	GLuint hasCompiled;

	shader = glCreateShader(shaderType);	
	
	if (shader == 0) return -1;
	
	//Cargamos los fuentes el shader
	glShaderSource (shader, 1, &shaderSrc, NULL);
	//Y los compilamos
	glCompileShader (shader);
	//Comprobamos que ha compilado correctamente, recuperando del objeto shader un array (vect) de enteros (iv)
	glGetShaderiv (shader, GL_COMPILE_STATUS, &hasCompiled);
	
	if (!hasCompiled){
		//Si no ha compilado correctamente, recuperamos otro dato del objeto shader que es una cadena 
		//con la causa del fallo.
		GLuint infoLen = 0;
		glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &infoLen);	
		char* infoLog = malloc (infoLen * sizeof(char));
		glGetShaderInfoLog (shader, infoLen, NULL, infoLog);
		printf ("Error de compilación de shader: %s\n", infoLog);
			
		return -1;
	}
	return shader;
} 

int init_shaders (){
	//Esta función carga los fuentes de los shaders, los manda a compilar, los coloca en el programObject
	//Recuerda: Compilar shaders, attacharlos al program object, linkar el program object
	GLuint vertexShader;
	GLuint fragmentShader;
	GLint isLinked;

	//Mandamos a compilar los fuentes de los shaders
	vertexShader = CompileShader (GL_VERTEX_SHADER, vertexShaderSrc);
	fragmentShader = CompileShader (GL_FRAGMENT_SHADER, fragmentShaderSrc);
	
	programObject = glCreateProgram();

	if (programObject == 0 ) {
		printf ("Error: no se pudo crear program object");
		return -1;
	}
	
	glAttachShader (programObject, vertexShader);
	glAttachShader (programObject, fragmentShader);

	glLinkProgram (programObject);

	glGetProgramiv (programObject, GL_LINK_STATUS, &isLinked);		
	
	//Si no ha linkado, recuperamos el mensaje de error del objeto programa
	if (!isLinked){
		char *infoLog;
		GLint infoLen = 0;
		glGetProgramiv (programObject, GL_INFO_LOG_LENGTH, &infoLen);
		infoLog = malloc (infoLen * sizeof(char));
		glGetProgramInfoLog (programObject, infoLen, NULL, infoLog);
		printf ("Error linkando objeto programa de los shaders: %s\n", infoLog);	
		return -1;
	}	
	
	glUseProgram (programObject);

	return 0;
}

void draw (){
		lbeLoadIdentity (&mvp);	

		lbeTranslate (&mvp, 0, 0, -6.0f);
		lbeRotate (&mvp, rAngle, 0, 1, 0);
		lbeMatrixMultiply (&mvp, &projection, &mvp);
		
		//Actualizamos en el shader el uniform de la matriz mvp 
		glUniformMatrix4fv(mvpOBJ, 1, GL_FALSE, &mvp.m[0][0]);
		
		glClear(GL_COLOR_BUFFER_BIT);
		
		//Dibujamos el cubo	
		draw_cube (0);	

		rAngle +=0.8f;
}

void main_loop (){
	while (1) {
		draw();
		flip_page();	
	}
}

int main(int argc, char *argv[])
{
	init_egl();
	
	init_shaders();

	//init_gl() se llama después de init_shaders() porque recupera datos de los mismos.	
	init_gl();

	//Establecemos el color de fondo para refrescar
	glClearColor (0.3f, 0.3f, 0.2f, 0.5f);	

	main_loop();	
	
	return 0;
}
