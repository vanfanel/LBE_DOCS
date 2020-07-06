#include "context.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define KMSDRM_DRI_PATH "/dev/dri/"

/* Helper functions, not to be used directly. */

static int check_modestting(int devindex)
{
    bool available = false;
    char device[512];
    int drm_fd;

    snprintf(device, sizeof (device), "%scard%d", KMSDRM_DRI_PATH, devindex);

    drm_fd = open(device, O_RDWR | O_CLOEXEC);
    if (drm_fd >= 0) {
	drmModeRes *resources = drmModeGetResources(drm_fd);
	if (resources) {
	    if (resources->count_connectors > 0 && resources->count_encoders > 0 && resources->count_crtcs > 0) {
		available = true;
	    }
	    drmModeFreeResources(resources);
	}
        close(drm_fd);
    }

    return available;
}

static int get_dricount(void)
{
    int devcount = 0;
    struct dirent *res;
    struct stat sb;
    DIR *folder;

    if (!(stat(KMSDRM_DRI_PATH, &sb) == 0
                && S_ISDIR(sb.st_mode))) {
        printf("The path %s cannot be opened or is not available\n",
               KMSDRM_DRI_PATH);
        return 0;
    }

    if (access(KMSDRM_DRI_PATH, F_OK) == -1) {
        printf("The path %s cannot be opened\n",
               KMSDRM_DRI_PATH);
        return 0;
    }

    folder = opendir(KMSDRM_DRI_PATH);
    if (folder) {
        while ((res = readdir(folder))) {
            int len = strlen(res->d_name);
            if (len > 4 && strncmp(res->d_name, "card", 4) == 0) {
                devcount++;
            }
        }
        closedir(folder);
    }

    return devcount;
}

static int
get_driindex(void)
{
    const int devcount = get_dricount();
    int i;

    for (i = 0; i < devcount; i++) {
        if (check_modestting(i)) {
            return i;
        }
    }

    return -ENOENT;
}

static int
KMSDRM_Available(void)
{
    int ret = -ENOENT;

    ret = get_driindex();
    if (ret >= 0)
        return 1;

    return ret;
}

static void
KMSDRM_FBDestroyCallback(struct gbm_bo *bo, void *data)
{
    KMSDRM_FBInfo *fb_info = (KMSDRM_FBInfo *)data;

    if (fb_info && fb_info->drm_fd >= 0 && fb_info->fb_id != 0) {
        drmModeRmFB(fb_info->drm_fd, fb_info->fb_id);
        printf("Delete DRM FB %u\n", fb_info->fb_id);
    }

    free(fb_info);
}

KMSDRM_FBInfo *
KMSDRM_FBFromBO(struct gbm_bo *bo)
{
    unsigned w,h;
    int ret;
    uint32_t stride, handle;

    /* Check for an existing framebuffer */
    KMSDRM_FBInfo *fb_info = (KMSDRM_FBInfo *)gbm_bo_get_user_data(bo);

    if (fb_info) {
        return fb_info;
    }

    /* Create a structure that contains enough info to remove the framebuffer
       when the backing buffer is destroyed */
    fb_info = (KMSDRM_FBInfo *)calloc(1, sizeof(KMSDRM_FBInfo));

    fb_info->drm_fd = viddata->drm_fd;

    /* Create framebuffer object for the buffer */
    w = gbm_bo_get_width(bo);
    h = gbm_bo_get_height(bo);
    stride = gbm_bo_get_stride(bo);
    handle = gbm_bo_get_handle(bo).u32;
    drmModeAddFB(viddata->drm_fd, w, h, 24, 32, stride, handle,
                                  &fb_info->fb_id);

    printf("New DRM FB (%u): %ux%u, stride %u from BO %p\n",
                 fb_info->fb_id, w, h, stride, (void *)bo);

    /* Associate our DRM framebuffer with this buffer object */
    gbm_bo_set_user_data(bo, fb_info, KMSDRM_FBDestroyCallback);

    return fb_info;
}

static void
KMSDRM_FlipHandler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data)
{
    /* If the data pointer received here is the same passed as the user_data in drmModePageFlip()
       then this is the event handler for the pageflip that was issued on drmPageFlip(): got here 
       because of that precise page flip, the while loop gets broken here because of the right event.
       This knowledge will allow handling different issued pageflips if sometime in the future 
       managing different CRTCs in SDL2 is needed, for example (synchronous pageflips happen on vblank 
       and vblank is a CRTC thing). */
    *((bool *) data) = false;
}

bool
KMSDRM_WaitPageFlip(int timeout) {
    drmEventContext ev = {0};
    struct pollfd pfd = {0};

    ev.version = DRM_EVENT_CONTEXT_VERSION;
    ev.page_flip_handler = KMSDRM_FlipHandler;

    pfd.fd = viddata->drm_fd;
    pfd.events = POLLIN;

    while (windata->waiting_for_flip) {
        pfd.revents = 0;

        if (poll(&pfd, 1, timeout) < 0) {
            printf("DRM poll error\n");
            return false;
        }

        if (pfd.revents & (POLLHUP | POLLERR)) {
            printf("DRM poll hup or error\n");
            return false;
        }

        /* Is the fd readable? Thats enough to call drmHandleEvent() on it. */
        if (pfd.revents & POLLIN) {
            /* Page flip? ONLY if the event that made the fd readable (=POLLIN state)
               is a page flip, will drmHandleEvent call page_flip_handler, which will break the loop.
               The drmHandleEvent() and subsequent page_flip_handler calls are both synchronous (blocking),
               nothing runs on a different thread, so no need to protect waiting_for_flip access with mutexes. */
            drmHandleEvent(viddata->drm_fd, &ev);
        } else {
            /* Timed out and page flip didn't happen */
            printf("Dropping frame while waiting_for_flip\n");
            return false;
        }
    }

    return true;
}


/* Usable video functions. */

void KMSDRM_DeleteDevice(void * device)
{

}

void KMSDRM_CreateDevice()
{

    int devindex = 0;

    if (!devindex || (devindex > 99)) {
        devindex = get_driindex();
    }

    if (devindex < 0) {
        printf("devindex (%d) must be between 0 and 99.\n", devindex);
        return;
    }

    viddata = (VideoData *) calloc(1, sizeof(VideoData));
    viddata->devindex = devindex;
    viddata->drm_fd = -1;
}

void
KMSDRM_DestroySurfaces()
{
    KMSDRM_WaitPageFlip(-1);

    if (windata->curr_bo) {
        gbm_surface_release_buffer(windata->gs, windata->curr_bo);
        windata->curr_bo = NULL;
    }

    if (windata->next_bo) {
        gbm_surface_release_buffer(windata->gs, windata->next_bo);
        windata->next_bo = NULL;
    }

    // Destroy EGL surface
    eglMakeCurrent(windata->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    if (windata->egl_surface != EGL_NO_SURFACE) {
        eglDestroySurface(windata->egl_display, windata->egl_surface);
        windata->egl_surface = EGL_NO_SURFACE;
    }
    
    // Destroy gbm surface
    if (windata->gs) {
        gbm_surface_destroy(windata->gs);
        windata->gs = NULL;
    }
}

void KMSDRM_CreateSurfaces()
{
    uint32_t width  = dispdata->mode.hdisplay;
    uint32_t height = dispdata->mode.vdisplay;
    uint32_t surface_fmt   = GBM_FORMAT_ARGB8888;
    uint32_t surface_flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING;
    EGLContext egl_context;
    EGLBoolean result;

    // GBM PART

    if (!gbm_device_is_format_supported(viddata->gbm_dev, surface_fmt, surface_flags)) {
        printf("GBM surface format not supported. Trying anyway.\n");
    }

    egl_context = (EGLContext)eglGetCurrentContext();

    KMSDRM_DestroySurfaces();

    windata->gs = gbm_surface_create(viddata->gbm_dev, width, height, surface_fmt, surface_flags);



    // EGL PART (using the GBM surface)

    static const EGLint attributeList[] = {
	    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	    EGL_RED_SIZE, 8,
	    EGL_GREEN_SIZE, 8,
	    EGL_BLUE_SIZE, 8,
	    EGL_ALPHA_SIZE, 8,
	    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	    EGL_NONE
    };
	
    static const EGLint contextAttributes[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
    };

    EGLint numConfig;

    windata->egl_display = eglGetDisplay((NativeDisplayType)viddata->gbm_dev);
    assert(windata->egl_display != EGL_NO_DISPLAY);

    result = eglInitialize(windata->egl_display, NULL, NULL);
    assert(EGL_FALSE != result);

    result = eglChooseConfig(windata->egl_display, attributeList, &(windata->egl_config), 1, &numConfig);
    assert(EGL_FALSE != result);

    result = eglBindAPI(EGL_OPENGL_ES_API);
    assert(EGL_FALSE != result);
    
    windata->egl_context = eglCreateContext(windata->egl_display, windata->egl_config, EGL_NO_CONTEXT, contextAttributes);
    assert(windata->egl_context != EGL_NO_CONTEXT);

    windata->egl_surface = eglCreateWindowSurface(windata->egl_display, windata->egl_config, 
		(EGLNativeWindowType) windata->gs, NULL);

    if (windata->egl_surface == EGL_NO_SURFACE) {
        printf("Could not create EGL window surface\n");
    }

    eglMakeCurrent(windata->egl_display, windata->egl_surface, windata->egl_surface, windata->egl_context);

    windata->crtc_setup_pending = true;
}

void KMSDRM_VideoInit()
{
    int ret = 0;
    drmModeRes *resources = NULL;
    drmModeEncoder *encoder = NULL;
    char devname[32];
    VideoModeData desktop_mode = {0};

    dispdata = (DisplayData *)calloc(1, sizeof(DisplayData));

    /* Open /dev/dri/cardNN */
    snprintf(devname, sizeof(devname), "/dev/dri/card%d", viddata->devindex);

    printf("Opening device %s\n", devname);
    viddata->drm_fd = open(devname, O_RDWR | O_CLOEXEC);

    if (viddata->drm_fd < 0) {
        printf("Could not open %s", devname);
        return;
    }

    printf("Opened DRM FD (%d)", viddata->drm_fd);

    viddata->gbm_dev = gbm_create_device(viddata->drm_fd);
    if (!viddata->gbm_dev) {
        printf("Couldn't create gbm device.\n");
        return;
    }

    /* Get all of the available connectors / devices / crtcs */
    resources = drmModeGetResources(viddata->drm_fd);
    if (!resources) {
        printf("drmModeGetResources(%d) failed", viddata->drm_fd);
    }

    for (int i = 0; i < resources->count_connectors; i++) {
        drmModeConnector *conn = drmModeGetConnector(viddata->drm_fd, resources->connectors[i]);

        if (!conn) {
            continue;
        }

        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes) {
            printf("Found connector %d with %d modes.",
                         conn->connector_id, conn->count_modes);
            dispdata->conn = conn;
            break;
        }

        drmModeFreeConnector(conn);
    }

    if (!dispdata->conn) {
        printf("No currently active connector found.\n");
        return;
    }

    /* Try to find the connector's current encoder */
    for (int i = 0; i < resources->count_encoders; i++) {
        encoder = drmModeGetEncoder(viddata->drm_fd, resources->encoders[i]);

        if (!encoder) {
          continue;
        }

        if (encoder->encoder_id == dispdata->conn->encoder_id) {
            printf( "Found encoder %d.\n", encoder->encoder_id);
            break;
        }

        drmModeFreeEncoder(encoder);
        encoder = NULL;
    }

    if (!encoder) {
        /* No encoder was connected, find the first supported one */
        for (int i = 0, j; i < resources->count_encoders; i++) {
            encoder = drmModeGetEncoder(viddata->drm_fd, resources->encoders[i]);

            if (!encoder) {
              continue;
            }

            for (j = 0; j < dispdata->conn->count_encoders; j++) {
                if (dispdata->conn->encoders[j] == encoder->encoder_id) {
                    break;
                }
            }

            if (j != dispdata->conn->count_encoders) {
              break;
            }

            drmModeFreeEncoder(encoder);
            encoder = NULL;
        }
    }

    if (!encoder) {
        printf("No connected encoder found.\n");
        return;
    }

    printf("Found encoder %d.\n", encoder->encoder_id);

    /* Try to find a CRTC connected to this encoder */
    dispdata->saved_crtc = drmModeGetCrtc(viddata->drm_fd, encoder->crtc_id);

    if (!dispdata->saved_crtc) {
        /* No CRTC was connected, find the first CRTC that can be connected */
        for (int i = 0; i < resources->count_crtcs; i++) {
            if (encoder->possible_crtcs & (1 << i)) {
                encoder->crtc_id = resources->crtcs[i];
                dispdata->saved_crtc = drmModeGetCrtc(viddata->drm_fd, encoder->crtc_id);
                break;
            }
        }
    }

    if (!dispdata->saved_crtc) {
        printf("No CRTC found.\n");
        return;
    }

    dispdata->crtc_id = encoder->crtc_id;

    /* Figure out the default mode to be set. If the current CRTC's mode isn't
       valid, select the first mode supported by the connector

       FIXME find first mode that specifies DRM_MODE_TYPE_PREFERRED */
    dispdata->mode = dispdata->saved_crtc->mode;

    if (dispdata->saved_crtc->mode_valid == 0) {
        printf("Current mode is invalid, selecting connector's mode #0.\n");
        dispdata->mode = dispdata->conn->modes[0];
    }

    /* DRM mode index for the desktop mode is needed to complete desktop mode init NOW,
       so look for it in the DRM modes array. */
    for (int i = 0; i < dispdata->conn->count_modes; i++) {
        if (!memcmp(dispdata->conn->modes + i, &dispdata->saved_crtc->mode, sizeof(drmModeModeInfo))) {
                desktop_mode.drm_mode_index = i;
        }   
    }   
}

void
KMSDRM_VideoQuit()
{
    /* Restore saved CRTC settings */
    if (viddata->drm_fd >= 0 && dispdata && dispdata->conn && dispdata->saved_crtc) {
        drmModeConnector *conn = dispdata->conn;
        drmModeCrtc *crtc = dispdata->saved_crtc;

        int ret = drmModeSetCrtc(viddata->drm_fd, crtc->crtc_id, crtc->buffer_id,
                                        crtc->x, crtc->y, &conn->connector_id, 1, &crtc->mode);

        if (ret != 0) {
            printf("Could not restore original CRTC mode\n");
        }
    }
    if (dispdata && dispdata->conn) {
        drmModeFreeConnector(dispdata->conn);
        dispdata->conn = NULL;
    }
    if (dispdata && dispdata->saved_crtc) {
        drmModeFreeCrtc(dispdata->saved_crtc);
        dispdata->saved_crtc = NULL;
    }
    if (viddata->gbm_dev) {
        gbm_device_destroy(viddata->gbm_dev);
        viddata->gbm_dev = NULL;
    }
    if (viddata->drm_fd >= 0) {
        close(viddata->drm_fd);
        printf("Closed DRM FD %d\n", viddata->drm_fd);
        viddata->drm_fd = -1;
    }
}

void KMSDRM_CreateWindow()
{
    /* Allocate window internal data */
    windata = (WindowData *)calloc(1, sizeof(WindowData));

    /* Create the surfaces for the window. */
    KMSDRM_CreateSurfaces();
}

void
KMSDRM_DestroyWindow()
{
    KMSDRM_DestroySurfaces();
    free(windata);
}

void KMSDRM_SwapWindow() {
    KMSDRM_FBInfo *fb_info;

    /* Ask EGL to mark the current back buffer to become the next front buffer. 
       That will happen when a pageflip is issued, and the next vsync arrives (sync flip)
       or ASAP (async flip). */
    if (!(eglSwapBuffers(windata->egl_display, windata->egl_surface))) {
	printf("eglSwapBuffers failed.\n");
	return;
    }

    /* Get a handler to the buffer that is marked to become the next front buffer, and lock it
       so it can not be chosen by EGL as a back buffer. */
    windata->next_bo = gbm_surface_lock_front_buffer(windata->gs);
    if (!windata->next_bo) {
	printf("Could not lock GBM surface front buffer\n");
	return;
    }

    /* Issue synchronous pageflip: drmModePageFlip() NEVER blocks, synchronous here means that it
       will be done on next VBLANK, not ASAP. And return to program loop inmediately. */

    fb_info = KMSDRM_FBFromBO(windata->next_bo);
    if (!fb_info) {
	return;
    }

    /* When needed, this is done once we have the needed fb_id, not before. */
    if (windata->crtc_setup_pending) {
	if (drmModeSetCrtc(viddata->drm_fd, dispdata->crtc_id, fb_info->fb_id, 0,
				    0, &dispdata->conn->connector_id, 1, &dispdata->mode)) {
	    printf("Could not configure CRTC on video mode setting.\n");
	}
        windata->crtc_setup_pending = false;
    }

    if (!drmModePageFlip(viddata->drm_fd, dispdata->crtc_id, fb_info->fb_id,
				DRM_MODE_PAGE_FLIP_EVENT, &windata->waiting_for_flip)) {
	windata->waiting_for_flip = true;
    } else {
	printf("Could not issue pageflip\n");
    }

    /* Since issued pageflips are always synchronous (ASYNC dont currently work), these pageflips
       will happen at next vsync, so in practice waiting for vsync is being done here. */ 
    if (!KMSDRM_WaitPageFlip(-1)) {
	printf("Error waiting for pageflip event\n");
	return;
    }

    /* Return the previous front buffer to the available buffer pool of the GBM surface,
       so it can be chosen again by EGL as the back buffer for drawing into it. */
    if (windata->curr_bo) {
	gbm_surface_release_buffer(windata->gs, windata->curr_bo);
	/* SDL_LogDebug(SDL_LOG_CATEGORY_VIDEO, "Released GBM surface buffer %p", (void *)windata->curr_bo); */
	windata->curr_bo = NULL;
    }

    /* Take note of the current front buffer, so it can be freed next time this function is called. */
    windata->curr_bo = windata->next_bo;
}
