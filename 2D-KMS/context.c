#include "context.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/mman.h>

// for PRIu64 that's neeeded to printf 64-bit unsigned vars.
#include <inttypes.h>

#include "pixformats.h"

// This is defined in Robclark's recent xf86drmMode.h but not on current Debian one,
// so this shouln't be necessary on future GNU/Linux systems.
#define DRM_MODE_OBJECT_PLANE 0xeeeeeeee

enum fill_pattern {
	PATTERN_TILES = 0,
	PATTERN_PLAIN = 1,
	PATTERN_SMPTE = 2,
};



int flip_page = 0;


void dump_planes (int fd);

void drmPageFlipHandler(int fd, uint frame, uint sec, uint usec, void *data) {
	int *waiting_for_flip = (int *)data;
	*waiting_for_flip = 0;
}

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

	// Programmer!! Save your sanity!!
	// VERY important or we won't get all the available planes on drmGetPlaneResources()!
	drmSetClientCap(drm.fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

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

bool isOverlay (drmModePlane *plane) {

	int i,j;
	
	// the property values and their names are stored in different arrays, so we
	// access them simultaneously here.
	// We are interested in OVERLAY planes only, that's type 0 or DRM_PLANE_TYPE_OVERLAY
	// (see /usr/xf86drmMode.h for definition).
	drmModeObjectPropertiesPtr props;	
	props = drmModeObjectGetProperties(drm.fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);

	for (j=0; j < props->count_props; j++) {
		if ( !strcmp(drmModeGetProperty(drm.fd, props->props[j])->name, "type")){
			// found the type property
			if (props->prop_values[j] == DRM_PLANE_TYPE_OVERLAY){
				return true;
			}
			else return false;
		}
	}
	printf ("\n");
}

void dump_planes (int fd) {

	int i,j;
	
	drmModePlaneRes *plane_resources;
	drmModePlane *plane;	
	char format_str[5];
	
	plane_resources = drmModeGetPlaneResources(drm.fd);
	
	printf ("Total available planes %d\n", plane_resources->count_planes);
	for (i = 0; i < plane_resources->count_planes; i++) {
		plane = drmModeGetPlane(fd, plane_resources->planes[i]);
		printf ("**Overlay ID %d supported pixel formats:**\n",plane->plane_id);
		// iterate over the supported pixel formats of each plane.
		for (j = 0; j < plane->count_formats; j++) {
			get_format_name(plane->formats[j], format_str);
			printf ("%s\t", format_str);	
		}	
		printf ("\n");
		
		// the property values and their names are stored in different arrays, so we
		// access them simultaneously here.
		drmModeObjectPropertiesPtr props;	
		props = drmModeObjectGetProperties(drm.fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);
		printf ("Plane properties: ");
		for (j=0; j < props->count_props; j++) {
			printf(" %s ",drmModeGetProperty(drm.fd, props->props[j])->name);
			printf(" %"PRIu64"\t", props->prop_values[j]);
		}
		printf ("\n");
	}
	
}

static bool format_support(const drmModePlanePtr ovr, uint32_t fmt)
{
	unsigned int i;

	for (i = 0; i < ovr->count_formats; ++i) {
		if (ovr->formats[i] == fmt)
			return true;
	}

	return false;
}

void setup_plane () {
	// Plane stuff: planes are bound to connectors/encoders.
	int i,j;
	//struct plane_arg p;

	// get plane resources
	drmModePlane *overlay;	
	drmModePlaneRes *plane_resources;
	plane_resources = drmModeGetPlaneResources(drm.fd);
	if (!plane_resources) {
		printf ("No scaling planes available!\n");
	}

	
	printf ("MAC Num planes on FD %d is %d\n", drm.fd, plane_resources->count_planes);	

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
	
	// Programmer!! Save your sanity!! Primary planes have to cover the entire CRTC, and if you
	// don't do that, you will get dmesg error "Plane must cover entire CRTC".
	// Also, primary planes can not be scaled.
	printf("CRTC ID %d, NUM PLANES %d\n", drm.encoder->crtc_id, plane_resources->count_planes);
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

	//dump_planes(drm.fd);	

	// note src coords (last 4 args) are in Q16 format
	// crtc_w and crtc_h are the final size with applied scale/ratio.
	// crtc_x and crtc_y are the position of the plane
	// pw and ph are the input size: the size of the area we read from the fb.
	uint32_t crtc_x = 0;
	uint32_t crtc_y = 0;
	uint32_t plane_flags = 0;
	
	uint32_t pw = 320;
	uint32_t ph = 200;
	uint32_t crtc_w = drm.mode->hdisplay;
	uint32_t crtc_h = drm.mode->vdisplay;

	uint32_t src_offsetx = 0;
	uint32_t src_offsety = 0;

	/*extern int drmModeSetPlane(int fd, uint32_t plane_id, uint32_t crtc_id,
			   uint32_t fb_id, uint32_t flags,
			   int32_t crtc_x, int32_t crtc_y,
			   uint32_t crtc_w, uint32_t crtc_h,
			   uint32_t src_x, uint32_t src_y,
			   uint32_t src_w, uint32_t src_h);
	*/
	/*printf ("Trying to set plane with ID %d\n", drm.plane_id);	
	if (drmModeSetPlane(drm.fd, drm.plane_id, drm.crtc_id, bufs[0].fb,
			    plane_flags, crtc_x, crtc_y, crtc_w, crtc_h,
			    src_offsetx<<16, src_offsety<<16, pw<<16, ph<<16)) {
		fprintf(stderr, "failed to enable plane: %s\n",
			strerror(errno));	
	}*/

	printf ("Trying to set plane with ID %d\n", drm.plane_id);	
	if (drmModeSetPlane(drm.fd, drm.plane_id, drm.crtc_id, bufs[0].fb,
			    plane_flags, 363, 184, crtc_w, crtc_h,
			    src_offsetx<<16, src_offsety<<16, pw<<16, ph<<16)) {
		fprintf(stderr, "failed to enable plane: %s\n",
			strerror(errno));	
	}


	printf ("crtc_x %d, crtc_y %d, crtc_w %d, crtc_h %d, pw %d, ph %d\n", crtc_x, crtc_y, crtc_w, crtc_h, pw, ph);

	/*if (drmModeSetPlane(drm.fd, drm.plane_id, drm.crtc_id, bufs[0].fb,
			    plane_flags, crtc_x, crtc_y, 320, 200,
			    0, 0, 320, 200)) {
		fprintf(stderr, "failed to enable plane: %s\n",
			strerror(errno));	
	}*/
}

void setup_plane2 () {
	int i,j;

	// get plane resources
	drmModePlane *plane;	
	drmModePlaneRes *plane_resources;
	plane_resources = drmModeGetPlaneResources(drm.fd);
	if (!plane_resources) {
		printf ("No scaling planes available!\n");
	}

	//printf ("Num planes on FD %d is %d\n", drm.fd, plane_resources->count_planes);	
	//dump_planes(drm.fd);	

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
	
	// Programmer!! Save your sanity!! Primary planes have to cover the entire CRTC, and if you
	// don't do that, you will get dmesg error "Plane must cover entire CRTC".
	// Look at linux/source/drivers/gpu/drm/drm_plane_helper.c comments for more info.
	// Also, primary planes can't be scaled: we need overlays for that.
	// printf("CRTC ID %d, NUM PLANES %d\n", drm.encoder->crtc_id, plane_resources->count_planes);
	for (i = 0; i < plane_resources->count_planes; i++) {
		plane = drmModeGetPlane(drm.fd, plane_resources->planes[i]);
		isOverlay(plane);
		if (!(plane->possible_crtcs & (1 << crtc_index))){
			printf ("plane with ID %d can't be used with current CRTC\n",plane->plane_id);
			continue;
		}
		if (!format_support(plane, bufs[0].pixel_format)) {
			printf ("plane with ID %d does not support framebuffer format\n", plane->plane_id);
			continue;
		}
		// we are only interested in overlay planes. No overlay, no fun. 
		// (no scaling, must cover crtc..etc) so we skip primary planes
		if (!isOverlay(plane)) {
			printf ("plane with ID %d is not an overlay: it's primary. Not usable.\n", plane->plane_id);
			continue;
		}	
                drm.plane_id = plane->plane_id;
		drmModeFreePlane(plane);
        }

	if (!drm.plane_id) {
		printf ("couldn't find an usable overlay plane for current CRTC and framebuffer pixel formal.\n");
		deinit_kms();
		exit (0);
	}
	else {
		printf ("using plane/overlay ID %d\n", drm.plane_id);
	}


	// note src coords (last 4 args) are in Q16 format
	// crtc_w and crtc_h are the final size with applied scale/ratio.
	// crtc_x and crtc_y are the position of the plane
	// pw and ph are the input size: the size of the area we read from the fb.
	uint32_t plane_flags = 0;
	uint32_t plane_w = 640;
	uint32_t plane_h = 480;
	uint32_t plane_x = 0;
	uint32_t plane_y = 0;
	
	uint32_t src_w = 320;
	uint32_t src_h = 200;
	uint32_t src_x = 0;
	uint32_t src_y = 0;

	/*extern int drmModeSetPlane(int fd, uint32_t plane_id, uint32_t crtc_id,
			   uint32_t fb_id, uint32_t flags,
			   int32_t crtc_x, int32_t crtc_y,
			   uint32_t crtc_w, uint32_t crtc_h,
			   uint32_t src_x, uint32_t src_y,
			   uint32_t src_w, uint32_t src_h);
	*/
	/*printf ("Trying to set plane with ID %d\n", drm.plane_id);	
	if (drmModeSetPlane(drm.fd, drm.plane_id, drm.crtc_id, bufs[0].fb,
			    plane_flags, crtc_x, crtc_y, crtc_w, crtc_h,
			    src_offsetx<<16, src_offsety<<16, pw<<16, ph<<16)) {
		fprintf(stderr, "failed to enable plane: %s\n",
			strerror(errno));	
	}*/

	printf ("Trying to set plane with ID %d on CRTC ID %d format %d\n", drm.plane_id, drm.crtc_id, bufs[0].pixel_format);	
	if (drmModeSetPlane(drm.fd, drm.plane_id, drm.crtc_id, bufs[0].fb,
			    plane_flags, plane_x, plane_y, plane_w, plane_h,
			    src_x<<16, src_y<<16, src_w<<16, src_h<<16)) {
		fprintf(stderr, "failed to enable plane: %s\n",
			strerror(errno));	
	}

	printf ("src_w %d, src_h %d, plane_w %d, plane_h %d\n", src_w, src_h, plane_w, plane_h);

	/*if (drmModeSetPlane(drm.fd, drm.plane_id, drm.crtc_id, bufs[0].fb,
			    plane_flags, crtc_x, crtc_y, 320, 200,
			    0, 0, 320, 200)) {
		fprintf(stderr, "failed to enable plane: %s\n",
			strerror(errno));	
	}*/
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
	creq.bpp = 16;
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

	buf->pixel_format = DRM_FORMAT_RGB565;
	//buf->pixel_format = DRM_FORMAT_XRGB8888;
	
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
        
	/*bufs[0].width = drm.mode->hdisplay;
	bufs[0].height = drm.mode->vdisplay;

	bufs[1].width = drm.mode->hdisplay;
	bufs[1].height = drm.mode->vdisplay;*/

	bufs[0].width = 320;
	bufs[0].height = 200;

	bufs[1].width = 320;
	bufs[1].height = 200;

	/* create framebuffer #1 for this CRTC */
	int ret = modeset_create_fb2(drm.fd, &bufs[0]);	
	if (ret) {
		printf("can't create fb\n");
	}

	/* create framebuffer #2 for this CRTC */
	ret = modeset_create_fb2(drm.fd, &bufs[1]);	
	if (ret) {
		printf("can't create fb\n");
	}

	// set mode physical video mode. We start scanout-ing on buffer 0.
        /*if (drmModeSetCrtc(drm.fd, drm.crtc_id, bufs[0].fb, 0, 0, &drm.connector_id, 1, drm.mode)) {
                printf ("failed to set mode\n");
                return;
        }*/

	setup_plane2();

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
