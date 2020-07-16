#include <stdbool.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <fcntl.h>

typedef struct VideoData
{
    int devindex;               /* device index that was passed on creation */
    int drm_fd;                 /* DRM file desc */
    struct gbm_device *gbm_dev;
} VideoData;

typedef struct
{
    int mode_index;
} DisplayModeData;

typedef struct VideoModeData
{
    int w;
    int h;
    int refresh_rate;
    int drm_mode_index;    

} VideoModeData;

typedef struct DisplayData
{
    uint32_t crtc_id;
    drmModeConnector *conn;
    drmModeModeInfo mode;
    drmModeModeInfo newmode;
    drmModeCrtc *saved_crtc;    /* CRTC to restore on quit */
} DisplayData;

typedef struct WindowData
{
    struct gbm_surface *gs;
    struct gbm_bo *curr_bo;
    struct gbm_bo *next_bo;
    struct gbm_bo *crtc_bo;
    bool waiting_for_flip;

    EGLSurface egl_surface;
    EGLDisplay egl_display;
    EGLConfig  egl_context;
    EGLConfig  egl_config;

    bool crtc_setup_pending;
} WindowData;

typedef struct KMSDRM_FBInfo
{
    int drm_fd;         /* DRM file desc */
    uint32_t fb_id;     /* DRM framebuffer ID */
} KMSDRM_FBInfo;

struct VideoData *viddata;
struct WindowData *windata;
struct DisplayData *dispdata;

/* Helper functions */
void KMSDRM_CreateDevice();
void KMSDRM_VideoInit();
void KMSDRM_VideoQuit();
void KMSDRM_CreateSurfaces();
void KMSDRM_DestroySurfaces();
void KMSDRM_CreateWindow();
void KMSDRM_DestroyWindow();
void KMSDRM_SwapWindow();
KMSDRM_FBInfo *KMSDRM_FBFromBO(struct gbm_bo *bo);
bool KMSDRM_WaitPageFlip(int timeout);
void drmPageFlip();
void KMSDRM_SetDisplayMode(int width);
