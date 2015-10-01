#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <fcntl.h>

struct {
	struct gbm_device *dev;
	struct gbm_surface *surface;
} gbm;

struct {
	int fd;
	drmModeModeInfo *mode;
	uint32_t crtc_id;
	uint32_t connector_id;

	drmModeCrtcPtr orig_crtc;
} drm;

struct drm_fb {
	struct gbm_bo *bo;
	uint32_t fb_id;
};

struct gbm_bo *bo;
struct drm_fb *fb;

struct {
	EGLDisplay display;
	EGLConfig config;
	EGLContext context;
	EGLSurface surface;
	uint width, height, refresh;
} eglInfo;

drmEventContext eventContext;

void init_egl();
void deinitEGL();
void drmPageFlip();
