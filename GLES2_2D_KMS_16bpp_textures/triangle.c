#include <stdbool.h>
#include "context.h"
#include <stdio.h>
#include <stdlib.h>

GLuint programObject;

GLuint LoadShader(GLenum type, const char *shaderSrc)
{
    GLuint shader;
    GLint compiled;
    // Create the shader object
    shader = glCreateShader(type);
    if(shader == 0)
	return 0;
    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);
    // Compile the shader
    glCompileShader(shader);
    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled)
	{
	    GLint infoLen = 0;
	    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
	    if(infoLen > 1)
		{
		    char* infoLog = malloc(sizeof(char) * infoLen);
		    glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
		    printf("Error compiling shader\n");
		    free(infoLog);
		}
	    glDeleteShader(shader);
	    return 0;
	}
    return shader;
}

int triangle_init ()
{
    GLbyte vShaderStr[] =
	"attribute vec4 vPosition;\n"
	"void main()\n"
	"{\n"
	"gl_Position = vPosition; \n"
	"}\n";
    GLbyte fShaderStr[] =
	"precision mediump float;\n"
	"void main()\n"
	"{\n"
	" gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); \n"
	"}\n";
    GLuint vertexShader;
    GLuint fragmentShader;

    GLint linked;

    // Load the vertex/fragment shaders
    vertexShader = LoadShader(GL_VERTEX_SHADER, vShaderStr);
    fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fShaderStr);
    // Create the program object
    programObject = glCreateProgram();
    if(programObject == 0)
	return 0;
    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);
    // Bind vPosition to attribute 0
    glBindAttribLocation(programObject, 0, "vPosition");
    // Link the program
    glLinkProgram(programObject);
    // Check the link status
    glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
    if(!linked)
	{
	    GLint infoLen = 0;
	    glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
	    if(infoLen > 1)
		{
		    char* infoLog = malloc(sizeof(char) * infoLen);
		    glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
		    printf("Error linking program\n");
		    free(infoLog);
		}
	    glDeleteProgram(programObject);
	    return false;
	}
    // Store the program object
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    return true;
}

void triangle_draw()
{
    GLfloat vVertices[] = {0.0f, 0.5f, 0.0f,
			   -0.5f, -0.5f, 0.0f,
			   0.5f, -0.5f, 0.0f};
    // Set the viewport
    glViewport(0, 0, eglInfo.width, eglInfo.height);
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);
    // Use the program object
    glUseProgram(programObject);
    // Load the vertex data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    eglSwapBuffers(eglInfo.display, eglInfo.surface);
}
