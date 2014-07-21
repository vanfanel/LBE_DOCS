#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "drmset.h"

struct drm_videodevice *drmDevice;

void macDraw (){
	//FASE4: dibujamos!
	int i, j, k;
	uint8_t r, g, b;
	unsigned int off;

	r = 0x00;
	g = 0xf0;
	b = 0x09;

	for (j = 0; j < drmDevice->height; ++j) {
		for (k = 0; k < drmDevice->width; ++k) {
	/*		off = drmDevice->stride * j + k * 4;
			*(uint32_t*)&drmDevice->map[off] =
				     (r << 16) | (g << 8) | b;
	*/
		macDrmPutPixel (k, j, r, g, b);	

		}
	}
}


int main (){
	macDrmSetup();	
	//macDraw();
	macDrmPutPixel (5, 5, 0xff, 0x00, 0x00);		
	getchar();
	/* cleanup everything */
	macDrmCleanup(drmDevice->fd);
}
