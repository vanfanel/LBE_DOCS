#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <gbm.h>

#include "context.h"

void cursor_test();

int main () {

	KMSDRM_CreateDevice();	
	KMSDRM_VideoInit();
        KMSDRM_CreateWindow();

        cursor_test();

	KMSDRM_SwapWindow();

	usleep(1000000);

        drmModeSetCursor(viddata->drm_fd, dispdata->crtc_id, 0, 0, 0);

        KMSDRM_DestroyWindow();
	KMSDRM_VideoQuit();
	return 0;
}

//Hardware GBM cursor creation test
void cursor_test () {
	
    struct gbm_bo *cursor_bo;
    uint32_t bo_format, bo_stride;
    size_t bufsize;
    char *buffer = NULL;
    uint32_t bo_handle;

    int cursor_w = 128;
    int cursor_h = 128;
            
    bo_format = GBM_FORMAT_ARGB8888;

    if (!gbm_device_is_format_supported(viddata->gbm_dev, bo_format, GBM_BO_USE_CURSOR | GBM_BO_USE_WRITE)) {
        printf("Unsupported pixel format for cursor\n");
    }

    cursor_bo = gbm_bo_create(viddata->gbm_dev, cursor_w, cursor_h, bo_format,
                                       GBM_BO_USE_CURSOR | GBM_BO_USE_WRITE);
    if (!cursor_bo) {
        printf("ERROR: Could not create GBM cursor BO\n");
    }
    else printf ("GBM cursor BO created successfully\n");

    bo_stride = gbm_bo_get_stride(cursor_bo);
    bufsize = bo_stride * cursor_h;

    /* Create a buffer and set all pixels of it. */
    buffer = malloc(bufsize);
    memset(buffer, 0xFFFFFFFF, bo_stride * cursor_h);

    /* Write the buffer to the cursor bo. */
    if (gbm_bo_write(cursor_bo, buffer, bufsize)) {
	printf("Could not write to GBM cursor BO\n");
    }

    /* Free buffer */
    free(buffer);
    buffer = NULL;

    /* Set cursor for CRTC */
    bo_handle = gbm_bo_get_handle(cursor_bo).u32;
    int ret = drmModeSetCursor(viddata->drm_fd, dispdata->crtc_id, bo_handle,
                               cursor_w, cursor_h);

    if (ret) {
        printf("drmModeSetCursor failed.\n");
    } else {
        printf("drmModeSetCursor succesfull.\n");
    }

    /* Hide cursor. */
    //drmModeSetCursor(viddata->drm_fd, dispdata->crtc_id, 0, 0, 0);
}
