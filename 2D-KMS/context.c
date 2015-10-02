#include "context.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

drmModePlaneRes *plane_resources;
struct modeset_buf bufs[2];	
int flip_page = 0;

void drmPageFlipHandler(int fd, uint frame, uint sec, uint usec, void *data) {
	int *waiting_for_flip = (int *)data;
	*waiting_for_flip = 0;
}

/*struct drm_fb *drmFBGetFromBO(struct gbm_bo *bo) {
	struct drm_fb *fb = gbm_bo_get_user_data(bo);
	uint32_t width, height, stride, handle;

	if (fb) {
		return fb;
	}
	
	fb = calloc(1, sizeof *fb);
	fb->bo = bo;

	width = gbm_bo_get_width(bo);
	height = gbm_bo_get_height(bo);
	stride = gbm_bo_get_stride(bo);
	handle = gbm_bo_get_handle(bo).u32;

	if (drmModeAddFB(drm.fd, width, height, 24, 32, stride, handle, &fb->fb_id)) {
		printf("Could not add drm framebuffer\n");
		free(fb);
		return NULL;
	}

	// We used to pass the destroy callback function here. Now it's done manually in deinitEGL()
	gbm_bo_set_user_data(bo, fb, NULL);
	return fb;
}*/

void drmPageFlip(void) {
	int waiting_for_flip = 1;
	fd_set fds;
	/* PROGRAMMER! SAVE YOUR SANITY with this information! Dark errors will engulf if you ignore this...
	 * A gbm surface has multiple buffers (a gbm surface is an "abstraction", a "window" backed by
	 * multiple buffers for double or triple buffering).
	 * So gbm_surface_lock_front_buffer() locks (forbids) rendering to the buffer that has became 
	 * the front buffer (visible buffer) AND gives us the next buffer to render into: hence it 
	 * MUST BE CALLED JUST AFTER eglSwapBuffers(), because we need a new buffer to render into.
	 * The best source of information for these functions is by Rob Clark himself on github:
	 * https://github.com/robclark/libgbm/blob/master/gbm.c
	 */ 

	/*struct gbm_bo *next_bo = gbm_surface_lock_front_buffer(gbm.surface);
	fb = drmFBGetFromBO(next_bo);*/

	if (drmModePageFlip(drm.fd, drm.crtc_id, bufs[flip_page].fb, DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip)) {
		printf ("Failed to queue pageflip\n");
		return;
	}

	while (waiting_for_flip) {
		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(drm.fd, &fds);
		select(drm.fd+1, &fds, NULL, NULL, NULL);
		drmHandleEvent(drm.fd, &eventContext);
	}

	// release last buffer to render on again
	/*gbm_surface_release_buffer(gbm.surface, bo);
	bo = next_bo;*/
	flip_page = !(flip_page);
}

bool initDRM(void) {
	eventContext.version = DRM_EVENT_CONTEXT_VERSION;
	eventContext.page_flip_handler = drmPageFlipHandler;

	drmModeConnector *connector;

	uint i, area;

	drm.fd = open("/dev/dri/card0", O_RDWR);

	if (drm.fd < 0) {
		printf ("could not open drm device\n");
		return false;
	}

	drmModeRes *resources = drmModeGetResources(drm.fd);
	if (!resources) {
		printf ("drmModeGetResources failed\n");
		return false;
	}

	// find a connected connector
	for (i = 0; i < (uint)resources->count_connectors; i++) {
		connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
		if (connector->connection == DRM_MODE_CONNECTED) {
			// it's connected, let's use this!
			break;
		}
		drmModeFreeConnector(connector);
		connector = NULL;
	}

	if (!connector) {
		// we could be fancy and listen for hotplug events and wait for
		// a connector..
		printf ("no connected connector found\n");
		return false;
	}
	// find highest resolution mode
	for (i = 0, area = 0; i < (uint)connector->count_modes; i++) {
		drmModeModeInfo *current_mode = &connector->modes[i];
		uint current_area = current_mode->hdisplay * current_mode->vdisplay;
		if (current_area > area) {
			drm.mode = current_mode;
			area = current_area;
		}
	}

	if (!drm.mode) {
		printf ("could not find mode\n");
		return false;
	}

	// find encoder
	for (i = 0; i < (uint)resources->count_encoders; i++) {
		drm.encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
		if (drm.encoder->encoder_id == connector->encoder_id)
			break;
		drmModeFreeEncoder(drm.encoder);
		drm.encoder = NULL;
	}

	if (!drm.encoder) {
		printf ("no encoder found\n");
		return false;
	}

	drm.crtc_id = drm.encoder->crtc_id;
	drm.connector_id = connector->connector_id;

	// backup original crtc so we can restore the original video mode on exit.
	drm.orig_crtc = drmModeGetCrtc(drm.fd, drm.encoder->crtc_id);

	

	return true;
}


void setup_overlay () {
	// Overlay stuff: overlays are bound to connectors/encoders.
	int i;

	// get plane resources
	drmModePlane *overlay;	
	plane_resources = drmModeGetPlaneResources(drm.fd);
	if (!plane_resources) {
		printf ("No scaling planes available!\n");
	}

	// look for a plane/overlay we can use with the original CRTC	
	printf("NUM PLANES %d\n", plane_resources->count_planes);
	for (i = 0; i < plane_resources->count_planes; i++) {
		overlay = drmModeGetPlane(drm.fd, plane_resources->planes[i]);
                if (overlay->possible_crtcs & drm.encoder->crtc_id){
                        drm.plane_id = overlay->plane_id;
			break;
		}
		drmModeFreePlane(overlay);
        }

	if (!drm.plane_id) {
		printf ("couldn't find an usable overlay for current CRTC\n");
	}

}
/*bool initGBM() {
	gbm.dev = gbm_create_device(drm.fd);

	gbm.surface = gbm_surface_create(gbm.dev,
			drm.mode->hdisplay, drm.mode->vdisplay,
			GBM_FORMAT_XRGB8888,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!gbm.surface) {
		printf ("failed to create gbm surface\n");
		return -1;
	}
	
	return true;
}*/

static int modeset_create_fb(int fd, struct modeset_buf *buf)
{
	struct drm_mode_create_dumb creq;
	struct drm_mode_destroy_dumb dreq;
	struct drm_mode_map_dumb mreq;
	int ret;

	/* create dumb buffer */
	memset(&creq, 0, sizeof(creq));
	creq.width = buf->width;
	creq.height = buf->height;
	creq.bpp = 16;
	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
	if (ret < 0) {
		printf("cannot create dumb buffer\n");
	}
	buf->stride = creq.pitch;
	buf->size = creq.size;
	buf->handle = creq.handle;

	/* create framebuffer object for the dumb-buffer */
	ret = drmModeAddFB(fd, buf->width, buf->height, 16, 16, buf->stride,
			   buf->handle, &buf->fb);
	if (ret) {
		printf("cannot create framebuffer\n");
		goto err_destroy;
	}

	/* prepare buffer for memory mapping */
	memset(&mreq, 0, sizeof(mreq));
	mreq.handle = buf->handle;
	ret = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
	if (ret) {
		printf("cannot map dumb buffer\n");
		goto err_fb;
	}

	/* perform actual memory mapping */
	buf->map = mmap(0, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		        fd, mreq.offset);
	if (buf->map == MAP_FAILED) {
		printf("cannot mmap dumb buffer\n");
		goto err_fb;
	}

	/* clear the framebuffer to 0 */
	memset(buf->map, 0, buf->size);

	return 0;

err_fb:
	drmModeRmFB(fd, buf->fb);
err_destroy:
	memset(&dreq, 0, sizeof(dreq));
	dreq.handle = buf->handle;
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
	return ret;
}

void init_kms() {
	if (!initDRM()) {
		printf ("failed to initialize DRM\n");
		return;
	}
	
	// Create the GBM surface, which contains several buffers. A GBM surface is an "abstraction".
	/*if (!initGBM()) {
		printf ("failed to initialize GBM\n");
		return;
	}*/
	
	//bo = gbm_surface_lock_front_buffer(gbm.surface);
        //fb = drmFBGetFromBO(bo);

	// BLOCK non-GL starts: in this block we do what we need to do because we don't use a
	// GBM surface anymore (hence we don't have the framebuffers of the GBM surface anymore) 
	// because we won't use GLES, but dummy buffers instead.	

	// ALT BLOCK
	// instead of a gbm bo, we add a dummy framebuffer this time
	// Structures for fb creation, memory mapping and destruction.
        
	bufs[0].width = drm.mode->hdisplay;
	bufs[0].height = drm.mode->vdisplay;

	bufs[1].width = drm.mode->hdisplay;
	bufs[1].height = drm.mode->vdisplay;

	/* create framebuffer #1 for this CRTC */
	int ret = modeset_create_fb(drm.fd, &bufs[0]);	
	if (ret) {
		printf("no valid crtc for connector\n");
	}

	/* create framebuffer #2 for this CRTC */
	ret = modeset_create_fb(drm.fd, &bufs[1]);	
	if (ret) {
		printf("no valid crtc for connector\n");
	}

	// ALT BLOCK ENDS
	

	// BLOCK non-GL ends

	// set mode physical video mode. We start scanout-ing on buffer 0.
        if (drmModeSetCrtc(drm.fd, drm.crtc_id, bufs[0].fb, 0, 0, &drm.connector_id, 1, drm.mode)) {
                printf ("failed to set mode\n");
                return;
        }

	setup_overlay();

	printf ("KMS init succesfull\n");
}

void drmDraw(void *pixels) {
	// soft blitting...
	int y, src_off, dst_off = 0;
	for (y = 0; y < 200; y++) {		
		dst_off = bufs[flip_page].stride * y;		
		src_off = 320 * 2 * y;
		memcpy (bufs[flip_page].map + dst_off, (uint8_t*)pixels + src_off, 320 * 2);
	}
}

void deinit_kms() {
	// Restore the original videomode/connector/scanoutbuffer(fb) combination (the original CRTC, that is). 
	drmModeSetCrtc(drm.fd, drm.orig_crtc->crtc_id, drm.orig_crtc->buffer_id,
		drm.orig_crtc->x, drm.orig_crtc->y,
		&drm.connector_id, 1, &drm.orig_crtc->mode);	

	if (bufs[0].fb) {
		drmModeRmFB(drm.fd, bufs[0].fb);
		drmModeRmFB(drm.fd, bufs[1].fb);
	}
}
