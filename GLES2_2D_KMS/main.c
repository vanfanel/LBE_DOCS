#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "context.h"
#include "gles.h"

extern struct dispmanx_vars *_dispvars;

void clear_screen (int width, int height, void* pixels) {
	// Clear screen
	
	int i;
	for (i = 0; i < width * height ; i++)
		((uint16_t *)pixels)[i] = 0x0000;
}

int main () {
	
	int i, j, k, m;
	int total_pitch = 320 * 2; /*2 bpp*/
	
	uint16_t *pixels = malloc (320 * 200 * sizeof(uint16_t));
	init_egl();

	gles2_init(320, 200, 16, false);

	clear_screen (320, 200,  pixels);

	int ret;	
	
	for (m = 0; m < 2; m++) {
		for (j = 0; j < 320 - 50; j++) {
			
			clear_screen (320, 200,  pixels);
			
			for (i = 0; i < 200; i++) {
				
				for (k = 0; k < 50; k++) 
					pixels[i * 320 + j + k] = 0x0FF0;
				
			}
			
			gles2_draw(pixels);
			ret = eglSwapBuffers(eglInfo.display, eglInfo.surface);
			drmPageFlip();
		}
	}
	free (pixels);
	deinitEGL();
	
	return 0;
}
