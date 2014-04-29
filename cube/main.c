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

//MAC Estas variables son alteradas desde la clase teclado
extern float rAngle, rSpeed, rSign;
extern int exit_condition;

unsigned* keystate;

//Aquí vamos a guardar ambos shaders una vez compilados: le hacemos un attach a ambos a este objeto.
GLuint programObject;

static int init_gl(void)
{
	return 0;
}

static void draw(uint32_t i)
{
	glDrawArrays(GL_TRIANGLES,0,3);
}

//Recuerda: los uniform son las variables que le pasamos desde el programa en C, 
//Las variables gl_* ya vienen definidas y son los valores que le pasamos de vuelta a GLES.
//Así, gl_Position es la posición final de un vértice (tras las transformaciones de cámara/modelo/proyección)
//y gl_fragColor es el color del píxel de cada fragment.
GLbyte vertexShaderSrc[] = 
	"attribute vec4 vertexPosition;		\n"
	"void main () {				\n"
	"	gl_Position = vertexPosition;	\n"
	"}					\n"
;

GLbyte fragmentShaderSrc[] = 
	"precision mediump float;			 \n"
	"void main (){				 	 \n"
	"	 gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
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

float vertices[] = {0.0f, 0.5f, 0.0f,
		   -0.5f, -0.5f, 0.0f,
		    0.5f, -0.5f, 0.0f};

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

	setupShaders();


	//Establecemos el viewport
	printf ("Usando modo de vídeo %d x %d\n", eglInfo.width, eglInfo.height);
	glViewport (0, 0, eglInfo.width, eglInfo.height);
	
	//MAC Bloque de paso de geometría a memoria de GLES

	//Triángulo con la misma longitud de base (1 unidad) que de altura
	//Dados en órden de RHS, situado en el plano Z=0
	float vertices[] = {0.0f, 0.5f, 0.0f,
			   -0.3f, -0.5f, 0.0f,
			    0.3f, -0.5f, 0.0f};
	
	//Creamos el buffer object para poder colocar el array de vértices en un lugar de memoria accesible para GLES
	GLuint vertexBuffer;
	glGenBuffers (1, &vertexBuffer);
	
	//Colocamos el array en memoria de GLES: lo bindamos al GL_ARRAY_BUFFER,y a través de eso le pasamos los dats
	glBindBuffer (GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData (GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Cogemos el número de atributo de vertexPosition en el shader. Alternativamente, podríamos especificar
	//el nuestro con glBindAttribLocation(), pero antes de linkar el programa.
	GLint attVertices;
	attVertices = glGetAttribLocation (programObject, "vertexPosition"); 
	
	//Activamos ese atributo para que pueda ser usado desde el vertex shader
	glEnableVertexAttribArray(attVertices);	

	//Aquí es donde pasamos el buffer de manera implícita: se leerán los datos del buffer que esté en este
	//momento bounded a GL_ARRAY_BUFFER (cosa que hicimos en glBindBuffer()).	
	glVertexAttribPointer(attVertices, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//Deshacemos el binding para no alterar los datos del buffer sin querer.
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glClearColor (0.3f, 0.3f, 0.2f, 1.0f);	
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	eglSwapBuffers(eglInfo.display, eglInfo.surface);
	//Sólo para el contexto DRM/KMS
	DRM_PageFlip();	

	getchar();

	return 0;
}
