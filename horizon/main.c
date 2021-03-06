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

float leftAccel, rightAccel;
int iter1, iter2;
GLenum err;
//MAC Estas variables son alteradas desde la clase teclado
extern float rAngle, rSpeed, rSign;
extern int exit_condition;

//Aquí vamos a guardar ambos shaders una vez compilados: le hacemos un attach a ambos a este objeto.
GLuint programObject;

//Las matrices que vamos a ir usando y sus object handles (los GLints)
lbeMatrix mvp;
GLint mvpOBJ;
lbeMatrix projection;

static int init_gl(void)
{
	




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

static void draw_horizon(float viewerZ){
/*	//La secuencia de líneas horizontales empezará siempre en el 
	GLFloat horizon[] = { 
		-3.0f, -3.0f, viewerZ - 4.0f,	
		 3.0f, 3.0f, viewerZ - 4.0f	
	};     
	
	glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(attPositions, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)horizonOffset);
	glBindBuffer (GL_ARRAY_BUFFER, 0);

	glDrawArrays(GL_LINES, 0, 2);*/
}

static void flip_page(){
	eglSwapBuffers(eglInfo.display, eglInfo.surface);
	//Sólo para el contexto DRM/KMS
	DRM_PageFlip();	

}


//Recuerda: los uniform son las variables que le pasamos desde el programa en C, 
//Los attributes se llaman así porque son atributos del vértice, que es lo que se dedica a procesar el shader,
//un vértice por cada vez que se ejecuta su código.
//Las variables gl_* ya vienen definidas y son los valores que le pasamos de vuelta a GLES.
//Así, gl_Position es la posición final de un vértice (tras las transformaciones de cámara/modelo/proyección)
//y gl_fragColor es el color del píxel de cada fragment.
//Este vertex shader hace lo mínimo que debe hacer: dar la posición en eye coordinates (clipping space) del vértice.
//Recuerda también que las entradas (attributes, uniforms) se pueden llamar como te de la gana. Sólo tienes que
//tener en cuenta que las salidas tienen nombres fijos: gl_Position en el vertex shader y gl_FragColor en el fragment.
//Recuerda además que un vértice tiene tres atributos: posición, color y normal.
//Recuerda además que para pasar información de un shader a otro se usan los varying.

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

int setupShaders (){
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
		/*glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
		glBufferSubData (GL_ARRAY_BUFFER, horizonOffset, sizeof(horizon), horizon);
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		*/
		
		lbeLoadIdentity (&mvp);	

		lbeTranslate (&mvp, 0, 0, -6.0f);
		lbeRotate (&mvp, rAngle, 0, 1, 0);
		lbeMatrixMultiply (&mvp, &projection, &mvp);
		
		//Actualizamos en el shader el uniform de la matriz mvp 
		glUniformMatrix4fv(mvpOBJ, 1, GL_FALSE, &mvp.m[0][0]);
		
		glClear(GL_COLOR_BUFFER_BIT);
		
		//Dibujamos el cubo	
		draw_cube (0);	
		
		//MAC Comentado para deshabilitar horizonte
		/*
		glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
		glVertexAttribPointer(attPositions, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)horizonOffset);
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		
		if (tDistance >= 8.0f){
			//getchar();
			//printf("Cambiamos línea más lejana: tDistance = %f lastChanged = %d \n"
			//, tDistance, oldestLine);

			horizon [oldestLine][2] = - rTranslate - 50.0f;
			horizon [oldestLine][5] = - rTranslate - 50.0f;
			
			oldestLine++;			
			if (oldestLine > 4) oldestLine = 0; 

			glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
			glBufferSubData (GL_ARRAY_BUFFER, horizonOffset, sizeof(horizon), horizon);
			glBindBuffer (GL_ARRAY_BUFFER, 0);
			
			tDistance = 0.0f;
		}

		glDrawArrays(GL_LINES, 0, 26);
		MAC FIN BLOQUE HORIZONTE COMENTADO*/

		flip_page();	
	/*	if (loops >= 2000)
			exit_condition = 1;
	
		loops++;*/
		rAngle +=0.8f;
		//tDistance += 0.2f;

		//rTranslate += 0.2f;
		
		//printf ("horizon + rTranslate = %f\n", horizon[2] + rTranslate );
		
		//Las rectas han recorrido 20 unidades; toca sustituir.
}

int main(int argc, char *argv[])
{
	int ret = init_drm();
	if (ret) {
		printf("failed to initialize DRM\n");
		return ret;
	}

	ret = init_gbm();
	if (ret) {
		printf("failed to initialize GBM\n");
		return ret;
	}

	ret = init_egl();
	if (ret) {
		printf("failed to initialize EGL\n");
		return ret;
	}

	ret = init_gl();
	if (ret) {
		printf("failed to initialize GLES\n");
		return ret;
	}

	ret = setupShaders();
	if (ret) {
		printf("failed to initialize shaders\n");
		return ret;
	}

	//Establecemos el viewport
	printf ("Usando modo de vídeo %d x %d\n", eglInfo.width, eglInfo.height);
	glViewport (0, 0, eglInfo.width, eglInfo.height);
	//MAC La superficie de proyección es de ratio 1:1. La pantalla en cambio es 16(width):9(height), 
	//por lo que las cosas aparecen demasiado anchas. El ratio para compensar es height/width	
	//ya que la altura es menor y nos interesa un ratio < 0.
	float ratio = (float)eglInfo.height / (float)eglInfo.width;
	printf ("Phisical ratio W/H: %f\n", ratio);
	
	/*static const float vertices[] = {0.0f, 0.5f, 0.0f,
		   -0.5f, -0.5f, 0.0f,
		    0.5f, -0.5f, 0.0f};
	*/


	/*GLfloat horizon[][6] = {{-20.0f, -4.0f, -10.0f,	20.0f, -4.0f, -10.0f},
				{-20.0f, -4.0f, -20.0f,	20.0f, -4.0f, -20.0f},
				{-20.0f, -4.0f, -30.0f,	20.0f, -4.0f, -30.0f},
				{-20.0f, -4.0f, -40.0f,	20.0f, -4.0f, -40.0f},
				{-20.0f, -4.0f, -50.0f,	20.0f, -4.0f, -50.0f},
	
				{-16.0f, -4.0f, -5.0f,	-16.0f, -4.0f, -20000.0f},
				{-12.0f, -4.0f, -5.0f,	-12.0f, -4.0f, -20000.0f},
				{-8.0f,  -4.0f, -5.0f,  -8.0f, -4.0f, -20000.0f},
				{-4.0f,  -4.0f, -5.0f,	-4.0f, -4.0f, -20000.0f},
				{ 0.0f,  -4.0f, -5.0f,	 0.0f, -4.0f, -20000.0f},
				{ 4.0f,  -4.0f, -5.0f,	 4.0f, -4.0f, -20000.0f},
				{ 8.0f,  -4.0f, -5.0f,	 8.0f, -4.0f, -20000.0f},
				{ 12.0f, -4.0f, -5.0f,	 12.0f, -4.0f, -20000.0f},
				{ 16.0f, -4.0f, -5.0f,	 16.0f, -4.0f, -20000.0f}
	};*/


	static const GLfloat vertices[] = {
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
	
	/*static const float vertices[] = {
		  2.0f,  -2.0f, -2.0f,
		 -2.0f,  -2.0f, -2.0f,
		 -2.0f,   2.0f, -2.0f
	};*/

	/*static const GLfloat gridColors[] = {
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f, // magenta
		0.0f, 1.0f, 0.0f // magenta
	};*/

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
	
	//Creamos el buffer object para poder colocar el array de vértices en un lugar de memoria accesible 
	//para GLES. Se usa para guardar todos los atributos de los vértices: posición, color, normal.
	GLuint vertexBuffer;
	glGenBuffers (1, &vertexBuffer);
	
	/*Colocamos el array en memoria de GLES: lo bindamos al GL_ARRAY_BUFFER,y a través de eso le pasamos
	los datos. El buffer object donde vamos a guardar los vértices va a tener, además de su posición, 
	su color asociado y si nos interesa incluso la normal de cada uno. Para ello, primero se inicializa 
	el buffer con la llamada a glBufferData(), y luego ya se meten los datos con glBufferSubData()
	Originalmente, como sólo había que subir el array de las posiciones, se hacía directamente en 
	glBufferData()*/
	//glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
	//glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
	/*Calculamos los offsets dentro del buffer object de cada uno de los atributos: posición, color, 
	normal. Usamos uintptr_t porque son enteros que nos garantizan que guardan un puntero sin problemas.
	Acostúmbrate a la conversión entre entero sin signo y puntero, porque un puntero es un entero sin 
	signo.*/
	uintptr_t positionsOffset = 0;
	uintptr_t colorsOffset = (sizeof(vertices));
	//uintptr_t horizonOffset = (sizeof(vertices) + sizeof(colors));

	//glBufferData (GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors) + sizeof(horizon), NULL, GL_STATIC_DRAW);
	glBufferData (GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(colors), NULL, GL_STATIC_DRAW);
	glBufferSubData (GL_ARRAY_BUFFER, positionsOffset, sizeof(vertices), vertices);
	glBufferSubData (GL_ARRAY_BUFFER, colorsOffset, sizeof(colors), colors);
	//glBufferSubData (GL_ARRAY_BUFFER, colorsOffset, sizeof(gridColors), gridColors);
	//glBufferSubData (GL_ARRAY_BUFFER, horizonOffset, sizeof(horizon), horizon);
	

	//Cogemos el número de atributo de vertexPosition en el código del vertex shader. 
	//Alternativamente, podríamos especificar el nuestro con glBindAttribLocation(), pero 
	//antes de linkar el programa.
	GLint attPositions;
	GLint attColors;
	attPositions = glGetAttribLocation (programObject, "vertexPosition"); 
	attColors = glGetAttribLocation (programObject, "vertexColor");
	

	//Activamos cada atributo para que pueda ser usado desde el vertex shader
	glEnableVertexAttribArray(attPositions);	
	glEnableVertexAttribArray(attColors);	

	//Aquí lo que hacemos es enlazar los atributos en el shader con las direcciones de cada uno de los 
	//arrays que tenemos ya en el buffer: posición, color, normal
	//RECUERDA: UN BUFFER OBJECT, HASTA TRES ATRIBUTOS POR CADA VÉRTICE QUE GUARDAMOS EN ÉL
	glVertexAttribPointer(attPositions, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)positionsOffset);
	glVertexAttribPointer(attColors, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)colorsOffset);

	//Deshacemos el binding para no alterar los datos del buffer sin querer.
	glBindBuffer(GL_ARRAY_BUFFER, 0);

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
	* serán tan pequeñas que no se verá, o quedarán fuera del frustrum por salirse la coordenada 
	* final del intervalo [-1,1], o cosas por el estilo. Así que resetea (carga la identidad) la 
	* MVP en cada iteración y, en lugar de rotar un mismo ángulo una y otra vez, rota con un 
	*ángulo cada vez mayor.
	*/
	float rAngle = 0.0f;
	//Cada vez que se recorra esta distancia, habrá que meter una nueva línea de horizonte.
	float tDistance = 0.0f;
	float rTranslate = 0.0f;

	//Última línea de geometría actualizada
	//int oldestLine = 0;	
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	//glDepthMask (GL_TRUE);	

	//Limpiamos el buffer de vídeo
	glClearColor (0.3f, 0.3f, 0.2f, 0.5f);	
	
	//Esto es espefícico de KMS. El contexto X11 entra en event_loop ya que X11 es un sistema de eventos.	
	while (!exit_condition) {
		draw();
	}
	
	return 0;
}
