#include "context.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

void drmPageFlipHandler(int fd, uint frame, uint sec, uint usec, void *data) {
	int *waiting_for_flip = (int *)data;
	*waiting_for_flip = 0;
}

struct drm_fb *drmFBGetFromBO(struct gbm_bo *bo) {
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

	struct gbm_bo *next_bo = gbm_surface_lock_front_buffer(gbm.surface);
	fb = drmFBGetFromBO(next_bo);

	if (drmModePageFlip(drm.fd, drm.crtc_id, fb->fb_id, DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip)) {
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
	gbm_surface_release_buffer(gbm.surface, bo);
	bo = next_bo;
}

bool initDRM(void) {
	// In plain C, we can just init _eventContext at declare time, but it's now allowed in C++
	eventContext.version = DRM_EVENT_CONTEXT_VERSION;
	eventContext.page_flip_handler = drmPageFlipHandler;

	drmModeConnector *connector;
	drmModeEncoder *encoder;
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
		encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
		if (encoder->encoder_id == connector->encoder_id)
			break;
		drmModeFreeEncoder(encoder);
		encoder = NULL;
	}

	if (!encoder) {
		printf ("no encoder found\n");
		return false;
	}

	drm.crtc_id = encoder->crtc_id;
	drm.connector_id = connector->connector_id;

	// backup original crtc so we can restore the original video mode on exit.
	drm.orig_crtc = drmModeGetCrtc(drm.fd, encoder->crtc_id);

	return true;
}

bool initGBM() {
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
}

void init_egl() {
	static const EGLint attributeList[] = {
	    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	    EGL_RED_SIZE, 8,
	    EGL_GREEN_SIZE, 8,
	    EGL_BLUE_SIZE, 8,
	    EGL_ALPHA_SIZE, 8,
	    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	    EGL_NONE
	};
	
	// create an EGL rendering context
	static const EGLint contextAttributes[] = {
	    EGL_CONTEXT_CLIENT_VERSION, 2,
	    EGL_NONE
	};

	EGLint numConfig;
	
	if (!initDRM()) {
		printf ("failed to initialize DRM\n");
		return;
	}

	if (!initGBM()) {
		printf ("failed to initialize GBM\n");
		return;
	}
	
	eglInfo.display = eglGetDisplay((NativeDisplayType)gbm.dev);
	assert(eglInfo.display != EGL_NO_DISPLAY);

	// initialize the EGL display connection
	EGLBoolean result = eglInitialize(eglInfo.display, NULL, NULL);
	assert(EGL_FALSE != result);
    
	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(eglInfo.display, attributeList, &eglInfo.config, 1, &numConfig);
	assert(EGL_FALSE != result);
    
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(EGL_FALSE != result);
	
	eglInfo.context = eglCreateContext(eglInfo.display, eglInfo.config, EGL_NO_CONTEXT, contextAttributes);
	assert(eglInfo.context != EGL_NO_CONTEXT);

        eglInfo.surface = eglCreateWindowSurface(eglInfo.display, eglInfo.config, 
		(EGLNativeWindowType) gbm.surface, NULL);
	assert(eglInfo.surface != EGL_NO_SURFACE);

	// connect the context to the surface
	result = eglMakeCurrent(eglInfo.display, eglInfo.surface, eglInfo.surface, eglInfo.context);
	assert(EGL_FALSE != result);

	eglSwapInterval(eglInfo.display, 1);
	
	eglSwapBuffers(eglInfo.display, eglInfo.surface);
	bo = gbm_surface_lock_front_buffer(gbm.surface);
        fb = drmFBGetFromBO(bo);

        // set mode physical video mode
        if (drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0, &drm.connector_id, 1, drm.mode)) {
                printf ("failed to set mode\n");
                return;
        }

        eglInfo.width = drm.mode->hdisplay;
        eglInfo.height = drm.mode->vdisplay;
        eglInfo.refresh = drm.mode->vrefresh;
	printf ("EGL init succesfull\n");
}

void deinitEGL() {
	// Release context resources
	eglMakeCurrent(eglInfo.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(eglInfo.display, eglInfo.surface);
	eglDestroyContext(eglInfo.display, eglInfo.context);
	eglTerminate(eglInfo.display);

	// Restore the original videomode/connector/scanoutbuffer combination (the original CRTC, that is). 
	drmModeSetCrtc(drm.fd, drm.orig_crtc->crtc_id, drm.orig_crtc->buffer_id,
		drm.orig_crtc->x, drm.orig_crtc->y,
		&drm.connector_id, 1, &drm.orig_crtc->mode);	

	if (fb->fb_id) {
		drmModeRmFB(drm.fd, fb->fb_id);
	}
	
	free(fb);
}
