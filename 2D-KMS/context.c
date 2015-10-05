#include "context.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/mman.h>
#include <drm/drm_fourcc.h>
#include "pixformats.h"

enum fill_pattern {
	PATTERN_TILES = 0,
	PATTERN_PLAIN = 1,
	PATTERN_SMPTE = 2,
};


struct modeset_buf bufs[2];	
int flip_page = 0;

/*struct plane_arg {
	uint32_t crtc_id;  // the id of CRTC to bind to
	bool has_position;
	int32_t x, y;
	uint32_t w, h;
	double scale;
	unsigned int fb_id;
	struct bo *bo;
	char format_str[5]; // need to leave room for terminating \0
	unsigned int fourcc;
};*/

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

	drm.resources = drmModeGetResources(drm.fd);
	if (!drm.resources) {
		printf ("drmModeGetResources failed\n");
		return false;
	}

	// find a connected connector
	for (i = 0; i < (uint)drm.resources->count_connectors; i++) {
		connector = drmModeGetConnector(drm.fd, drm.resources->connectors[i]);
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
	for (i = 0; i < (uint)drm.resources->count_encoders; i++) {
		drm.encoder = drmModeGetEncoder(drm.fd, drm.resources->encoders[i]);
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

// gets fourcc, returns name string.
void get_format_name(const unsigned int fourcc, char *format_str)
{
	unsigned int i;
	for (i = 0; i < ARRAY_SIZE(format_info); i++) {
		if (format_info[i].format == fourcc) {
			strcpy(format_str, format_info[i].name);
		}
	}
}

void dump_planes (int fd) {
	int i,j;
	
	drmModePlaneRes *plane_resources;
	drmModePlane *overlay;	
	char format_str[5];
	
	plane_resources = drmModeGetPlaneResources(drm.fd);
	
	printf ("Total available planes %d\n", plane_resources->count_planes);
	for (i = 0; i < plane_resources->count_planes; i++) {
		overlay = drmModeGetPlane(fd, plane_resources->planes[i]);
		printf ("**Overlay ID %d supported pixel formats:**\n",overlay->plane_id);
		for (j = 0; j < overlay->count_formats; j++) {
			get_format_name(overlay->formats[j], format_str);
			printf ("%s\t", format_str);	
		}	
		printf ("\n");
	}
}

void setup_overlay () {
	// Overlay stuff: overlays are bound to connectors/encoders.
	int i,j;
	//struct plane_arg p;

	// get plane resources
	drmModePlane *overlay;	
	drmModePlaneRes *plane_resources;
	plane_resources = drmModeGetPlaneResources(drm.fd);
	if (!plane_resources) {
		printf ("No scaling planes available!\n");
	}

	// look for a plane/overlay we can use with the configured CRTC	
	// Find a  plane which can be connected to our CRTC. Find the
	// CRTC index first, then iterate over available planes.
	// Yes, strangely we need the in-use CRTC index to mask possible_crtc 
	// during the planes iteration...

	unsigned int crtc_index;
	//struct crtc *crtc = NULL;
	for (i = 0; i < (unsigned int)drm.resources->count_crtcs; i++) {
		if (drm.crtc_id == drm.resources->crtcs[i]) {
			crtc_index = i;
			printf ("CRTC index found %d with ID %d\n", crtc_index, drm.crtc_id);
			break;
		}
	}
	

	printf("NUM PLANES %d, CRTC ID %d\n", plane_resources->count_planes, drm.encoder->crtc_id);
	for (i = 0; i < plane_resources->count_planes; i++) {
		overlay = drmModeGetPlane(drm.fd, plane_resources->planes[i]);
                if (overlay->possible_crtcs & (1 << crtc_index)){
                        drm.plane_id = overlay->plane_id;
			printf ("using plane/overlay ID %d\n", drm.plane_id);
			break;
		}
		drmModeFreePlane(overlay);
        }

	if (!drm.plane_id) {
		printf ("couldn't find an usable overlay for current CRTC\n");
		deinit_kms();
		exit (0);
	}

	// iterate over the supported pixel formats of the overlay
	// TODO: use an abstract window struct type so we don't have to chose a buffer to compare here, but
	// look at the pixel format of the window struct. A window has several buffers but all buffers will have
	// the same pixel format. The pixel format of a buffer is specified at the AddFB2() function call. 
	char format_str[5];
	/*printf ("Overlay ID %d supported pixel formats:\n",drm.plane_id);
	for (i = 0; i < overlay->count_formats; ++i) {
		get_format_name(overlay->formats[i], format_str);
		printf ("%s\n", format_str);	
		if (overlay->formats[i] == bufs[0].pixel_format)
			printf ("FOUND!\n");
	}*/

	dump_planes(drm.fd);	

	// note src coords (last 4 args) are in Q16 format
	// crtc_w and crtc_h are the final size with applied scale/ratio.
	// pw and ph are the input size: the size of the area we read from the fb.
	int crtc_x = 0;
	int crtc_y = 0;
	uint32_t plane_flags = 0;
	
	int pw = 320;
	int ph = 200;
	int crtc_w = drm.mode->hdisplay;
	int crtc_h = drm.mode->vdisplay;
	
	if (drmModeSetPlane(drm.fd, drm.plane_id, drm.crtc_id, bufs[0].fb,
			    plane_flags, /*crtc_x*/0, /*crtc_y*/0, crtc_w, crtc_h,
			    0, 0, pw << 16, ph << 16)) {
		fprintf(stderr, "failed to enable plane: %s\n",
			strerror(errno));	
	}
}

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

	/* create framebuffer object for the dumb-buffer. We have the 
	 * framebuffer and we need an object to work with it. 	*/
	ret = drmModeAddFB(fd, buf->width, buf->height, 16, 16, buf->stride,
			   buf->handle, &buf->fb);
	if (ret) {
		printf("cannot create framebuffer object\n");
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

/* Implementation using drmModeAddFB2() */
static int modeset_create_fb2(int fd, struct modeset_buf *buf)
{
	struct drm_mode_create_dumb creq;
	struct drm_mode_destroy_dumb dreq;
	struct drm_mode_map_dumb mreq;
	int ret;

	// create dumb buffer
	memset(&creq, 0, sizeof(creq));
	creq.width = buf->width;
	creq.height = buf->height;
	creq.bpp = 32;
	ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
	if (ret < 0) {
		printf("cannot create dumb buffer\n");
	}
	buf->stride = creq.pitch;
	buf->size = creq.size;
	buf->handle = creq.handle;

	// create framebuffer object for the dumb-buffer. We have the 
	// framebuffer and we need an object to work with it.
	//ret = drmModeAddFB(fd, buf->width, buf->height, 16, 16, buf->stride,
	//	buf->handle, &buf->fb);

	//buf->pixel_format = DRM_FORMAT_RGB565;
	buf->pixel_format = DRM_FORMAT_XRGB8888;
	
	uint32_t offsets[1];
	offsets[1] = 0;
	ret = drmModeAddFB2(fd, buf->width, buf->height, 
		buf->pixel_format, &buf->handle, &buf->stride, offsets, &buf->fb, 0);
 
	if (ret) {
		printf("cannot create framebuffer object\n");
		goto err_destroy;
	}

	// prepare buffer for memory mapping
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
	
	// BLOCK non-GL starts: in this block we do what we need to do because we don't use a
	// GBM surface anymore (hence we don't have the framebuffers of the GBM surface anymore) 
	// because we won't use GLES, but dummy buffers instead.	

	// instead of a gbm bo, we add a dummy framebuffer this time
	// Structures for fb creation, memory mapping and destruction.
        
	bufs[0].width = drm.mode->hdisplay;
	bufs[0].height = drm.mode->vdisplay;

	bufs[1].width = drm.mode->hdisplay;
	bufs[1].height = drm.mode->vdisplay;

	/* create framebuffer #1 for this CRTC */
	int ret = modeset_create_fb2(drm.fd, &bufs[0]);	
	if (ret) {
		printf("no valid crtc for connector\n");
	}

	/* create framebuffer #2 for this CRTC */
	ret = modeset_create_fb2(drm.fd, &bufs[1]);	
	if (ret) {
		printf("no valid crtc for connector\n");
	}

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
