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

/*typedef struct VideoModeData
{
    int w;
    int h;
    int refresh_rate;
    int drm_mode_index;    

} VideoModeData;*/

typedef struct DisplayData
{
    uint32_t crtc_id;
    drmModeConnector *conn;
    drmModeModeInfo mode;
    drmModeCrtc *saved_crtc;    /* CRTC to restore on quit */
} DisplayData;

struct VideoData *viddata;
struct DisplayData *dispdata;

/* Helper functions */
void KMSDRM_CreateDevice();
void KMSDRM_VideoInit();
void KMSDRM_VideoQuit();
