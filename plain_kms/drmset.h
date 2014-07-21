#include <xf86drm.h>
#include <xf86drmMode.h>

/*#define USE_LIBKMS*/
#ifdef USE_LIBKMS 
	#include <libkms/libkms.h>
#endif


typedef struct drm_videodevice {
	uint32_t fd;
	uint32_t width;
	uint32_t height;
	uint32_t stride;
	uint32_t size;
	unsigned handle;
	uint8_t *mappedmem;

	struct kms_bo *bo[2];	
	struct kms_driver *kms;

	drmModeModeInfo mode;
	uint32_t fb;
	drmModeRes *resources;
	drmModeCrtc *saved_crtc;
	drmModeCrtc *crtc;
	drmModeEncoder *encoder; 
	drmModeConnector *connector;


	//Bloque de variables para overlay
	uint32_t src_width, src_height, src_offsetx, src_offsety;

} drm_videodevice;

int macDrmSetup ();
void macDrmCleanup ();
void macDrmPutPixel( int x, int y, uint8_t r, uint8_t g, uint8_t b);
static struct kms_bo *drmAllocateBuffer(struct kms_driver *kms, int width, int height, int *stride);
