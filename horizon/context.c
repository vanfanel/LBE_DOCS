#include <context.h>
#include <gbm.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

drmEventContext evctx = {
			.version = DRM_EVENT_CONTEXT_VERSION,
			.page_flip_handler = page_flip_handler,
};

size_t buflen = 20;
char buf[80];

int init_drm(void)
{
	static const char *modules[] = {
			"i915", "radeon", "nouveau", "vmwgfx", "omapdrm", "exynos", "msm"
	};
	drmModeRes *resources;
	drmModeConnector *connector = NULL;
	drmModeEncoder *encoder = NULL;
	int i, area;

	for (i = 0; i < ARRAY_SIZE(modules); i++) {
		printf("trying to load module %s...", modules[i]);
		drm.fd = drmOpen(modules[i], NULL);
		if (drm.fd < 0) {
			printf("failed.\n");
		} else {
			printf("success.\n");
			break;
		}
	}

	if (drm.fd < 0) {
		printf("could not open drm device\n");
		return -1;
	}

	resources = drmModeGetResources(drm.fd);
	if (!resources) {
		strerror_r (errno, buf, buflen);
		printf("drmModeGetResources failed: %s\n",buf);
		return -1;
	}

	/* find a connected connector: */
	for (i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(drm.fd, resources->connectors[i]);
		if (connector->connection == DRM_MODE_CONNECTED) {
			/* it's connected, let's use this! */
			break;
		}
		drmModeFreeConnector(connector);
		connector = NULL;
	}

	if (!connector) {
		/* we could be fancy and listen for hotplug events and wait for
		 * a connector..
		 */
		printf("no connected connector!\n");
		return -1;
	}

	/* find highest resolution mode: */
	for (i = 0, area = 0; i < connector->count_modes; i++) {
		drmModeModeInfo *current_mode = &connector->modes[i];
		int current_area = current_mode->hdisplay * current_mode->vdisplay;
		if (current_area > area) {
			drm.mode = current_mode;
			area = current_area;
		}
	}

	if (!drm.mode) {
		printf("could not find mode!\n");
		return -1;
	}

	/* find encoder: */
	for (i = 0; i < resources->count_encoders; i++) {
		encoder = drmModeGetEncoder(drm.fd, resources->encoders[i]);
		if (encoder->encoder_id == connector->encoder_id)
			break;
		drmModeFreeEncoder(encoder);
		encoder = NULL;
	}

	if (!encoder) {
		printf("no encoder!\n");
		return -1;
	}

	drm.crtc_id = encoder->crtc_id;
	drm.connector_id = connector->connector_id;

	return 0;
}

int init_gbm(void)
{
	gbm.dev = gbm_create_device(drm.fd);

	gbm.surface = gbm_surface_create(gbm.dev,
			drm.mode->hdisplay, drm.mode->vdisplay,
			GBM_FORMAT_XRGB8888,
			GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	if (!gbm.surface) {
		printf("failed to create gbm surface\n");
		return -1;
	}

	return 0;
}

void drm_fb_destroy_callback(struct gbm_bo *bo, void *data)
{
	struct drm_fb *fb = data;
	struct gbm_device *gbm = gbm_bo_get_device(bo);

	if (fb->fb_id)
		drmModeRmFB(drm.fd, fb->fb_id);

	free(fb);
}

struct drm_fb * drm_fb_get_from_bo(struct gbm_bo *bo)
{
	struct drm_fb *fb = gbm_bo_get_user_data(bo);
	uint32_t width, height, stride, handle;
	int ret;

	if (fb)
		return fb;

	fb = calloc(1, sizeof *fb);
	fb->bo = bo;

	width = gbm_bo_get_width(bo);
	height = gbm_bo_get_height(bo);
	stride = gbm_bo_get_stride(bo);
	handle = gbm_bo_get_handle(bo).u32;

	ret = drmModeAddFB(drm.fd, width, height, 24, 32, stride, handle, &fb->fb_id);
	if (ret) {
		strerror_r (errno, buf, buflen);
		printf("failed to create fb: %s\n", buf);
		free(fb);
		return NULL;
	}

	gbm_bo_set_user_data(bo, fb, drm_fb_destroy_callback);

	return fb;
}

int init_egl(void) {

	EGLint major, minor, n;
	
	static const EGLint context_attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	static const EGLint config_attribs[] = {
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RED_SIZE, 1,
		EGL_GREEN_SIZE, 1,
		EGL_BLUE_SIZE, 1,
		EGL_ALPHA_SIZE, 0,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	eglInfo.display = eglGetDisplay((NativeDisplayType)gbm.dev);

	if (!eglInitialize(eglInfo.display, &major, &minor)) {
		printf("failed to initialize\n");
		return -1;
	}

	printf("Using display %p with EGL version %d.%d\n",
			eglInfo.display, major, minor);

	printf("EGL Version \"%s\"\n", eglQueryString(eglInfo.display, EGL_VERSION));
	printf("EGL Vendor \"%s\"\n", eglQueryString(eglInfo.display, EGL_VENDOR));
	printf("EGL Extensions \"%s\"\n", eglQueryString(eglInfo.display, EGL_EXTENSIONS));

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		printf("failed to bind api EGL_OPENGL_ES_API\n");
		return -1;
	}

	if (!eglChooseConfig(eglInfo.display, config_attribs, &eglInfo.config, 1, &n) || n != 1) {
		printf("failed to choose config: %d\n", n);
		return -1;
	}

	eglInfo.context = eglCreateContext(eglInfo.display, eglInfo.config,
			EGL_NO_CONTEXT, context_attribs);
	if (eglInfo.context == NULL) {
		printf("failed to create context\n");
		return -1;
	}

	eglInfo.surface = eglCreateWindowSurface(eglInfo.display, eglInfo.config, 
		(EGLNativeWindowType) gbm.surface, NULL);
	if (eglInfo.surface == EGL_NO_SURFACE) {
		printf("failed to create egl surface\n");
		return -1;
	}

	/* connect the context to the surface */
	eglMakeCurrent(eglInfo.display, eglInfo.surface, eglInfo.surface, eglInfo.context);

	//MAC Sacar esto a otra función
	int ret;

	eglSwapBuffers(eglInfo.display, eglInfo.surface);
	bo = gbm_surface_lock_front_buffer(gbm.surface);
	fb = drm_fb_get_from_bo(bo);

	/* set mode: */
	ret = drmModeSetCrtc(drm.fd, drm.crtc_id, fb->fb_id, 0, 0,
			&drm.connector_id, 1, drm.mode);
	if (ret) {
		strerror_r (errno, buf, buflen);
		printf("failed to set mode: %s\n", buf);
		return ret;
	}
	
	eglInfo.width = drm.mode->hdisplay;
	eglInfo.height = drm.mode->vdisplay;
	eglInfo.refresh = drm.mode->vrefresh;
	//MAC Fin bloque

	return 0;
}

int DRM_PageFlip(void){
	int ret;		
	struct gbm_bo *next_bo;
	int waiting_for_flip = 1;

	next_bo = gbm_surface_lock_front_buffer(gbm.surface);
	fb = drm_fb_get_from_bo(next_bo);

	/*
	 * Here you could also update drm plane layers if you want
	 * hw composition
	 */

	ret = drmModePageFlip(drm.fd, drm.crtc_id, fb->fb_id,
			DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);
	if (ret) {
		strerror_r (errno, buf, buflen);
		printf("failed to queue page flip: %s\n", buf);
		return -1;
	}

	while (waiting_for_flip) {
		//Aquí realmente no nos interesa leer el teclado. Sólo
		//hacemos el select para esperar por drm.fd
		FD_ZERO (&fds);
		
		FD_SET (0, &fds);
		FD_SET (drm.fd, &fds);

		ret = select(drm.fd+1, &fds, NULL, NULL, NULL);
		
		drmHandleEvent(drm.fd, &evctx);
	}

	/* release last buffer to render on again: */
	gbm_surface_release_buffer(gbm.surface, bo);
	bo = next_bo;
	
	return 0;
}
