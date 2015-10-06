#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <fcntl.h>

struct {
	struct gbm_device *dev;
	struct gbm_surface *surface;
} gbm;

struct drm_struct {
	int fd;
	drmModeModeInfo *mode;
	uint32_t crtc_id;
	uint32_t connector_id;

	drmModeCrtcPtr orig_crtc;

	uint32_t plane_id;	

	drmModeEncoder *encoder;
	drmModeRes *resources;
};

struct modeset_buf {
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t size;
	uint32_t handle;
	uint8_t *map;
	uint32_t fb;
	uint32_t pixel_format;
};

struct drm_struct drm;
struct modeset_buf bufs[2];	

drmEventContext eventContext;

void init_kms();
void deinit_kms();
void drmDraw(void *pixels);
void drmPageFlip();
