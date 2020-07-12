#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <gbm.h>

#include "context.h"

void create_cursor();
void destroy_cursor();
void alpha_premultiply_ARGB8888 (uint32_t *pixel);

int main () {

	KMSDRM_CreateDevice();	
	KMSDRM_VideoInit();

        create_cursor();

	usleep(1000000);

        destroy_cursor();

	KMSDRM_VideoQuit();

	return 0;
}

//Hardware GBM cursor creation test
void create_cursor () {
	
    struct gbm_bo *cursor_bo;
    uint32_t bo_format, bo_stride;
    size_t bufsize;
    char *buffer = NULL;
    uint32_t bo_handle;

    int cursor_w = 128;
    int cursor_h = 128;
    /* Each line has 128 pixels, 32 bits per pixel => each line has 4096 bits,
       so each line is 4096 bits / 8 = 512 bytes.
       Total buffer size (bufsize) => 512 * 128 = 65536 bytes. 
    */
            
    bo_format = GBM_FORMAT_ARGB8888;

    if (!gbm_device_is_format_supported(viddata->gbm_dev, bo_format, GBM_BO_USE_CURSOR | GBM_BO_USE_WRITE)) {
        printf("Unsupported pixel format for cursor\n");
    }

    cursor_bo = gbm_bo_create(viddata->gbm_dev, cursor_w, cursor_h, bo_format, GBM_BO_USE_CURSOR | GBM_BO_USE_WRITE);
    if (!cursor_bo) {
        printf("ERROR: Could not create GBM cursor BO\n");
    }
    else printf ("GBM cursor BO created successfully\n");

    bo_stride = gbm_bo_get_stride(cursor_bo);
    bufsize = bo_stride * cursor_h;

    /* Create a buffer and set all pixels of it. */
    buffer = malloc(bufsize);
    printf ("bo_stride = %d buffsize = %d\n", bo_stride, (int)bufsize);

    /* VERY important: We have created an ARGB8888 GBM BO for the cursor,
       but we need to fill it with alpha-premultiplied pìxels [AA, RR, GG, BB], instead of straight-alpha ones
       [AA. AA*RR, AA*GG, AA*BB]. */ 
    uint32_t pixel = 0xEEFF0000;
    alpha_premultiply_ARGB8888 (&pixel);

    /* */
    for (int i = 0; i < (bufsize/4); i++) {
        memcpy(((uint32_t*)buffer) + i, &pixel, 4);
    }

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
}

/* Each pixel is converted from straight-alpha [AA, RR, GG, BB] to premultiplied-alpha [AA. AA*RR, AA*GG, AA*BB].
   But we have to operate these multiplications with floats instead of integers, and also we have 
   to convert the values to be relative to 0-255, where 255 is 1.00 and anything between 0 and 255 is 0.xx.
*/
void alpha_premultiply_ARGB8888 (uint32_t *pixel) {
    uint32_t pix_output = 0;
    uint32_t A, R, G, B;

    A = (*pixel >> (3 << 3)) & 0xFF;
    R = (*pixel >> (2 << 3)) & 0xFF;
    G = (*pixel >> (1 << 3)) & 0xFF;
    B = (*pixel >> (0 << 3)) & 0xFF;

    printf ("Extracted A byte = 0x%x = float %2f\n", A, (float)A /255 );
    printf ("Extracted R byte = 0x%x = float %2f\n", R, (float)R /255 );
    printf ("Extracted G byte = 0x%x = float %2f\n", G, (float)G /255 );
    printf ("Extracted B byte = 0x%x = float %2f\n", B, (float)B /255 );
    printf ("\n\n");

    printf("Composed pix_output (before ALPHA premult.) = 0x%x\n", *pixel);

    R = (float)A * ((float)R /255);
    G = (float)A * ((float)G /255);
    B = (float)A * ((float)B /255);

    (*pixel) = (((uint32_t)A << 24) | ((uint32_t)R << 16) | ((uint32_t)G << 8)) | ((uint32_t)B << 0);

    printf ("ALPHA-multiplied A byte = 0x%x = float %2f\n", A, (float)A /255 );
    printf ("ALPHA-multiplied R byte = 0x%x = float %2f\n", R, (float)R /255 );
    printf ("ALPHA-multiplied G byte = 0x%x = float %2f\n", G, (float)G /255 );
    printf ("ALPHA-multiplied B byte = 0x%x = float %2f\n", B, (float)B /255 );
    printf ("\n\n");

    printf("Composed pix_output (after ALPHA premult.) = 0x%x\n", *pixel);
}

void destroy_cursor () {
        drmModeSetCursor(viddata->drm_fd, dispdata->crtc_id, 0, 0, 0);
}
