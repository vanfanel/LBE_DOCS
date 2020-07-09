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

void KMSDRM_VideoInit()
{
    int ret = 0;
    drmModeRes *resources = NULL;
    drmModeEncoder *encoder = NULL;
    char devname[32];

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
}

void
KMSDRM_VideoQuit()
{
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
