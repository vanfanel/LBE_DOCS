#ifndef CONTEXT_H
#define CONTEXT_H

#include <GLES2/gl2.h>
#include <EGL/egl.h>

struct {
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
	int width, height, refresh;
} eglInfo;


struct {
	Window win;
	Display *x_dpy;
} xInfo;

#endif
