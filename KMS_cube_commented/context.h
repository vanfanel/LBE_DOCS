#ifndef CONTEXT_H
#define CONTEXT_H

#include <xf86drm.h>
#include <drm/drm_mode.h>
#include <xf86drmMode.h>

#include <GLES2/gl2.h>
#include <EGL/egl.h>

int init_drm(void);

int init_gbm(void);

struct {
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
	GLuint program;
	GLint modelviewmatrix, modelviewprojectionmatrix, normalmatrix;
	GLuint vbo;
	GLuint positionsoffset, colorsoffset, normalsoffset;
} gl;

struct {
	struct gbm_device *dev;
	struct gbm_surface *surface;
} gbm;

struct {
	int fd;
	drmModeModeInfo *mode;
	uint32_t crtc_id;
	uint32_t connector_id;
} drm;

struct drm_fb {
	struct gbm_bo *bo;
	uint32_t fb_id;
};

struct gbm_bo *bo;
struct drm_fb *fb;
fd_set fds;

static void page_flip_handler(int fd, unsigned int frame,
		  unsigned int sec, unsigned int usec, void *data)
{
	int *waiting_for_flip = data;
	*waiting_for_flip = 0;
}

drmEventContext evctx;

#endif
