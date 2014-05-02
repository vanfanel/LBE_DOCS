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

	//static const GLfloat vVertices[] = {

	GLfloat vVertices[ 7512 ] = {
		// front
		-1.0f, 0.5f, +1.0f, // point blue
		+1.0f, 0.5f, +1.0f, // point magenta
		-1.0f, 1.0f, +1.0f, // point cyan
		+1.0f, 1.0f, +1.0f, // point white
	
	};

	static const GLfloat vColors[] = {
			// front
			0.0f,  0.0f,  1.0f, // blue
			1.0f,  0.0f,  1.0f, // magenta
			0.0f,  1.0f,  1.0f, // cyan
			1.0f,  1.0f,  1.0f, // white
	};

	static const GLfloat vNormals[] = {
			// front
			+0.0f, +0.0f, +1.0f, // forward
			+0.0f, +0.0f, +1.0f, // forward
			+0.0f, +0.0f, +1.0f, // forward
			+0.0f, +0.0f, +1.0f, // forward
		

	};

	static const char *vertex_shader_source =
			"uniform mat4 modelviewMatrix;      \n"
			"uniform mat4 modelviewprojectionMatrix;\n"
			"uniform mat3 normalMatrix;         \n"
			"                                   \n"
			"attribute vec4 in_position;        \n"
			"attribute vec3 in_normal;          \n"
			"attribute vec4 in_color;           \n"
			"\n"
			"vec4 lightSource = vec4(2.0, 2.0, 20.0, 0.0);\n"
			"                                   \n"
			"varying vec4 vVaryingColor;        \n"
			"                                   \n"
			"void main()                        \n"
			"{                                  \n"
			"    gl_Position = modelviewprojectionMatrix * in_position;\n"
			"    vec3 vEyeNormal = normalMatrix * in_normal;\n"
			"    vec4 vPosition4 = modelviewMatrix * in_position;\n"
			"    vec3 vPosition3 = vPosition4.xyz / vPosition4.w;\n"
			"    vec3 vLightDir = normalize(lightSource.xyz - vPosition3);\n"
			"    float diff = max(0.0, dot(vEyeNormal, vLightDir));\n"
			"    vVaryingColor = vec4(diff * in_color.rgb, 1.0);\n"
			"	 gl_PointSize = 10.0;\n" 
			"}                                  \n";

	static const char *fragment_shader_source =
			"precision mediump float;           \n"
			"                                   \n"
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

	gl.modelviewmatrix = glGetUniformLocation(gl.program, "modelviewMatrix");
	gl.modelviewprojectionmatrix = glGetUniformLocation(gl.program, "modelviewprojectionMatrix");
	gl.normalmatrix = glGetUniformLocation(gl.program, "normalMatrix");

	glViewport(0, 0, drm.mode->hdisplay, drm.mode->vdisplay);
	//glEnable(GL_CULL_FACE);
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
	
	//nave
	ESMatrix modelview;
	esMatrixLoadIdentity(&modelview);
	esTranslate(&modelview, 0.0f, 0.0f, -8.0f);
	rAngle = rAngle + rSpeed;
	esRotate(&modelview, -rAngle*0.1 , 0.0f, 0.0f, 1.0f);
	GLfloat aspect = (GLfloat)(drm.mode->vdisplay) / (GLfloat)(drm.mode->hdisplay);
	ESMatrix projection;
	esMatrixLoadIdentity(&projection);
	esFrustum(&projection, -2.8f, +2.8f, -2.8f * aspect, +2.8f * aspect, 4.0f, 10.0f);
	
	ESMatrix modelviewprojection;
	esMatrixLoadIdentity(&modelviewprojection);
	esMatrixMultiply(&modelviewprojection, &modelview, &projection);
	
	float normal[9];
	normal[0] = modelview.m[0][0];
	normal[1] = modelview.m[0][1];
	normal[2] = modelview.m[0][2];
	normal[3] = modelview.m[1][0];
	normal[4] = modelview.m[1][1];
	normal[5] = modelview.m[1][2];
	normal[6] = modelview.m[2][0];
	normal[7] = modelview.m[2][1];
	normal[8] = modelview.m[2][2];
	
	glUniformMatrix4fv(gl.modelviewmatrix, 1, GL_FALSE, &modelview.m[0][0]);
	glUniformMatrix4fv(gl.modelviewprojectionmatrix, 1, GL_FALSE, &modelviewprojection.m[0][0]);
	glUniformMatrix3fv(gl.normalmatrix, 1, GL_FALSE, normal);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	//suelo
	esMatrixLoadIdentity(&modelview);
	esTranslate(&modelview, 0.0f, 0.0f, -8.0f+i*0.05f);
	
	esRotate(&modelview, rAngle*0.1 , 0.0f, 0.0f, 1.0f);
	esMatrixLoadIdentity(&projection);
	esFrustum(&projection, -2.8f, +2.8f, -2.8f * aspect, +2.8f * aspect, 1.0f, 20.0f);
	esMatrixLoadIdentity(&modelviewprojection);
	esMatrixMultiply(&modelviewprojection, &modelview, &projection);
	normal[0] = modelview.m[0][0];
	normal[1] = modelview.m[0][1];
	normal[2] = modelview.m[0][2];
	normal[3] = modelview.m[1][0];
	normal[4] = modelview.m[1][1];
	normal[5] = modelview.m[1][2];
	normal[6] = modelview.m[2][0];
	normal[7] = modelview.m[2][1];
	normal[8] = modelview.m[2][2];
	glUniformMatrix4fv(gl.modelviewmatrix, 1, GL_FALSE, &modelview.m[0][0]);
	glUniformMatrix4fv(gl.modelviewprojectionmatrix, 1, GL_FALSE, &modelviewprojection.m[0][0]);
	glUniformMatrix3fv(gl.normalmatrix, 1, GL_FALSE, normal);
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
