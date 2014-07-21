#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "context.h"
#include "esUtil.h"

#include "teclado.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

float leftAccel, rightAccel;
int iter1, iter2;

//MAC Estas variables son alteradas desde la clase teclado
extern float rAngle, rSpeed, rSign;
extern int exit_condition;

unsigned* keystate;

//FILE *fp;

static int init_gl(void)
{
	GLuint vertex_shader, fragment_shader;
	GLint ret;

	static const GLfloat vVertices[] = {
		// front
		-1.0f, -1.0f, +1.0f, // point blue
		+1.0f, -1.0f, +1.0f, // point magenta
		-1.0f, +1.0f, +1.0f, // point cyan
		+1.0f, +1.0f, +1.0f, // point white
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
		-1.0f, +1.0f, +1.0f, // point cyan
		// top
		-1.0f, +1.0f, +1.0f, // point cyan
		+1.0f, +1.0f, +1.0f, // point white
		-1.0f, +1.0f, -1.0f, // point green
		+1.0f, +1.0f, -1.0f, // point yellow
		// bottom
		-1.0f, -1.0f, -1.0f, // point black
		+1.0f, -1.0f, -1.0f, // point red
		-1.0f, -1.0f, +1.0f, // point blue
		+1.0f, -1.0f, +1.0f // point magenta*/
	};
	
	static const GLfloat vColors[] = {
		// front
		1.0f, 0.0f, 0.0f, // blue
		1.0f, 0.0f, 0.0f, // magenta
		1.0f, 0.0f, 0.0f, // cyan
		1.0f, 0.0f, 0.0f, // white
		// back
		0.0f, 1.0f, 0.0f, // red
		0.0f, 1.0f, 0.0f, // black
		0.0f, 1.0f, 0.0f, // yellow
		0.0f, 1.0f, 0.0f, // green
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

	/*static const GLfloat vColors[] = {
		// front
		0.0f, 0.0f, 1.0f, // blue
		1.0f, 0.0f, 1.0f, // magenta
		0.0f, 1.0f, 1.0f, // cyan
		1.0f, 1.0f, 1.0f, // white
		// back
		1.0f, 0.0f, 0.0f, // red
		0.0f, 0.0f, 0.0f, // black
		1.0f, 1.0f, 0.0f, // yellow
		0.0f, 1.0f, 0.0f, // green
		// right
		1.0f, 0.0f, 1.0f, // magenta
		1.0f, 0.0f, 0.0f, // red
		1.0f, 1.0f, 1.0f, // white
		1.0f, 1.0f, 0.0f, // yellow
		// left
		0.0f, 0.0f, 0.0f, // black
		0.0f, 0.0f, 1.0f, // blue
		0.0f, 1.0f, 0.0f, // green
		0.0f, 1.0f, 1.0f, // cyan
		// top
		0.0f, 1.0f, 1.0f, // cyan
		1.0f, 1.0f, 1.0f, // white
		0.0f, 1.0f, 0.0f, // green
		1.0f, 1.0f, 0.0f, // yellow
		// bottom
		0.0f, 0.0f, 0.0f, // black
		1.0f, 0.0f, 0.0f, // red
		0.0f, 0.0f, 1.0f, // blue
		1.0f, 0.0f, 1.0f // magenta
	};*/

	static const GLfloat vNormals[] = {
		// front
		+0.0f, +0.0f, +1.0f, // forward
		+0.0f, +0.0f, +1.0f, // forward
		+0.0f, +0.0f, +1.0f, // forward
		+0.0f, +0.0f, +1.0f, // forward
		// back
		+0.0f, +0.0f, -1.0f, // backbard
		+0.0f, +0.0f, -1.0f, // backbard
		+0.0f, +0.0f, -1.0f, // backbard
		+0.0f, +0.0f, -1.0f, // backbard
		// right
		+1.0f, +0.0f, +0.0f, // right
		+1.0f, +0.0f, +0.0f, // right
		+1.0f, +0.0f, +0.0f, // right
		+1.0f, +0.0f, +0.0f, // right
		// left
		-1.0f, +0.0f, +0.0f, // left
		-1.0f, +0.0f, +0.0f, // left
		-1.0f, +0.0f, +0.0f, // left
		-1.0f, +0.0f, +0.0f, // left
		// top
		+0.0f, +1.0f, +0.0f, // up
		+0.0f, +1.0f, +0.0f, // up
		+0.0f, +1.0f, +0.0f, // up
		+0.0f, +1.0f, +0.0f, // up
		// bottom
		+0.0f, -1.0f, +0.0f, // down
		+0.0f, -1.0f, +0.0f, // down
		+0.0f, -1.0f, +0.0f, // down
		+0.0f, -1.0f, +0.0f // down
	};


	//MAC Recuerda que las matrices están guardadas como column-major porque es lo que GLSL espera,
	//y que la posición inicial del vértice es columna. Sin embargo, como GLSL va leyendo por columnas
	//la matriz MVP, se puede premultiplicar a la posición inicial del vértice como es lógico en álgebra.
	//DEBES desacoplar en tu cabeza el cómo se almacenan las matrices de cuáles siguen siendo las matrices
	//y de cómo se opera con ellas. Siempre intenta mantener la coherencia del álgebra, donde las transforma
	//ciones premultiplican a los vértices, y donde cada transformación sucesiva va más a la izquierda.
	//Y no te lies: esto es ya una vez abstraidos de si estamos en ASSU o en ASL. Eso ya está decidido a 
	//este nivel y la secuencia de transformaciones es la que es, se decidió antes.
	static const char *vertex_shader_source =
			"uniform mat4 modelviewprojectionMatrix;\n"
			"attribute vec4 in_position;        \n"
			"attribute vec4 in_color;           \n"
			"varying vec4 vVaryingColor;        \n"
			"                                   \n"
			"void main()                        \n"
			"{                                  \n"
			"    gl_Position = modelviewprojectionMatrix * in_position;\n"
			"    vVaryingColor = vec4(in_color.rgb, 1.0);\n"
			"}                                  \n";

	static const char *fragment_shader_source =
			"precision mediump float;           \n"
			"varying vec4 vVaryingColor;        \n"
			"                                   \n"
			"void main()                        \n"
			"{                                  \n"
			"    gl_FragColor = vVaryingColor;  \n"
			"}                                  \n";


	vertex_shader = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("vertex shader compilation failed!:\n");
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &ret);
		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(vertex_shader, ret, NULL, log);
			printf("%s", log);
		}

		return -1;
	}

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("fragment shader compilation failed!:\n");
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetShaderInfoLog(fragment_shader, ret, NULL, log);
			printf("%s", log);
		}

		return -1;
	}

	gl.program = glCreateProgram();

	glAttachShader(gl.program, vertex_shader);
	glAttachShader(gl.program, fragment_shader);

	glBindAttribLocation(gl.program, 0, "in_position");
	glBindAttribLocation(gl.program, 1, "in_normal");
	glBindAttribLocation(gl.program, 2, "in_color");

	glLinkProgram(gl.program);

	glGetProgramiv(gl.program, GL_LINK_STATUS, &ret);
	if (!ret) {
		char *log;

		printf("program linking failed!:\n");
		glGetProgramiv(gl.program, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = malloc(ret);
			glGetProgramInfoLog(gl.program, ret, NULL, log);
			printf("%s", log);
		}

		return -1;
	}

	glUseProgram(gl.program);

	//gl.modelviewmatrix = glGetUniformLocation(gl.program, "modelviewMatrix");
	gl.modelviewprojectionmatrix = glGetUniformLocation(gl.program, "modelviewprojectionMatrix");
	gl.normalmatrix = glGetUniformLocation(gl.program, "normalMatrix");

	glViewport(0, 0, drm.mode->hdisplay, drm.mode->vdisplay);
	glEnable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST); //ver caras ocultas
	//glEnable(GL_VERTEX_PROGRAM_POINT_SIZE); //Habilitar ancho puntos
	
	gl.positionsoffset = 0;
	gl.colorsoffset = sizeof(vVertices);
	gl.normalsoffset = sizeof(vVertices) + sizeof(vColors);
	glGenBuffers(1, &gl.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
	//Esta llamada a glBufferData() sólo inicializa: los datos se suben al buffer object en las llamadas a 
	//glBufferSubData() sucesivas.
	glBufferData(GL_ARRAY_BUFFER, sizeof(vVertices) + sizeof(vColors) + sizeof(vNormals), 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, gl.positionsoffset, sizeof(vVertices), &vVertices[0]);
	glBufferSubData(GL_ARRAY_BUFFER, gl.colorsoffset, sizeof(vColors), &vColors[0]);
	glBufferSubData(GL_ARRAY_BUFFER, gl.normalsoffset, sizeof(vNormals), &vNormals[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)gl.positionsoffset);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)gl.normalsoffset);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)gl.colorsoffset);
	glEnableVertexAttribArray(2);

	return 0;
}

static void draw(uint32_t i)
{
	glClearColor(0.2, 0.2, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	ESMatrix modelview;
	esMatrixLoadIdentity(&modelview);
	esTranslate(&modelview, 0.0f, 0.0f, -15.0f);
	rAngle = rAngle + rSpeed;
	esRotate(&modelview, -rAngle*0.1 , 0.0f, 1.0f, 0.0f);
	GLfloat aspect = (GLfloat)(drm.mode->vdisplay) / (GLfloat)(drm.mode->hdisplay);
	ESMatrix projection;
	esMatrixLoadIdentity(&projection);
	
	esFrustum(&projection, -2.8f, +2.8f, -2.8f * aspect, +2.8f * aspect, 6.0f, 30.0f);
	
	ESMatrix modelviewprojection;
	esMatrixLoadIdentity(&modelviewprojection);

	//modelview y projection están, desde el punto de vista de cómo está implementada la función
	//producto esMatrixMultiply(), traspuestas. Por eso se componen en órden contrario.
	//Y se le pasa tal cual a glUniform() porque los elementos ya están en memoria (NO en la matriz, que es 
	//la que es en papel) como los espea GLSL para hacer "gl_position = M*v", en ese órden. Si no te parece
	//matemáticamente coherente, es porque la coherencia matemática tienes que dejarla en el papel. En C
	// y GLSL simplemente se almacenan acceden los datos de una determinada forma, independientemente 
	//de las mates: y concretamente GLSL accede a la matriz de tal manera que sus elementos tienen que estar
	//una columna tras otra en memoria o, lo que es lo mismo, en el vector de 16 elementos equivalente a la
	//matriz 4x4.
	esMatrixMultiply(&modelviewprojection, &modelview, &projection);


/*	float normal[9];
	normal[0] = modelview.m[0][0];
	normal[1] = modelview.m[0][1];
	normal[2] = modelview.m[0][2];
	normal[3] = modelview.m[1][0];
	normal[4] = modelview.m[1][1];
	normal[5] = modelview.m[1][2];
	normal[6] = modelview.m[2][0];
	normal[7] = modelview.m[2][1];
	normal[8] = modelview.m[2][2];
*/	
	glUniformMatrix4fv(gl.modelviewprojectionmatrix, 1, GL_FALSE, &modelviewprojection.m[0][0]);
	//glUniformMatrix4fv(gl.modelviewprojectionmatrix, 1, GL_FALSE, &modelview.m[0][0]);
	
//	glUniformMatrix3fv(gl.normalmatrix, 1, GL_FALSE, normal);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);
	glDrawArrays(GL_TRIANGLE_STRIP, 12, 4);
}



int main(int argc, char *argv[])
{
	uint32_t i = 0;
	int ret;

	ret = init_drm();
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

	setupKeyboard();
        //MAC RECUERDA que restoreKeyboard NO se llama, se le pasa a atexit().
	atexit (restoreKeyboard);

	exit_condition = 0;
	leftAccel = 1;
	rightAccel = 1;

	unsigned timeStep = 32000; //Leemos cada 32ms (30 veces por segundo en un display de 60HZ). Como gettimeofday()
				 //sólo nos ofrece segundos y microsegundos, usaremos el campo de los microsegundos
				 //que son 10^-6 y lo multiplicaremos por mil para milisegundos (10^-3).

	struct timeval timeLast, timeCurrent; 
	unsigned timeDelta, timeElapsed, timeAccum;
	gettimeofday (&timeCurrent, NULL);
	
	//fp = fopen("log.txt", "w");
	while (1) {
		
		timeLast = timeCurrent;
		gettimeofday (&timeCurrent, NULL);
		timeDelta = timeCurrent.tv_usec - timeLast.tv_usec;
		timeAccum = timeAccum + timeDelta;

		if (timeAccum > timeStep){	
			//fprintf (fp, "Processing input...\n");
			timeAccum = 0;
			readKeyboard();
			//MAC Es importante que se retorne 0 y no se salga de otro modo,
			//con el fin de que se ejecute el callback de destrucción de buffer drm.
			//Por eso no salimos usando el while con exit_condition, sino retornando aquí.
			if (exit_condition)
				return 0;
		}

		draw(iter1++);
		
		eglSwapBuffers(gl.display, gl.surface);
		//Sólo para el contexto DRM/KMS
		DRM_PageFlip();	

	}

	//fclose (fp);
	return ret;
}
