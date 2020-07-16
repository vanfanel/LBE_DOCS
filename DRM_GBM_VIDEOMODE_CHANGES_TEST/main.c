#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <gbm.h>

#include "context.h"

void create_cursor ();
void destroy_cursor ();

int main () {

	KMSDRM_CreateDevice();	
	KMSDRM_VideoInit();
        KMSDRM_CreateWindow();

	KMSDRM_SetDisplayMode(640);
	KMSDRM_SwapWindow();
	usleep(1000000);

	KMSDRM_SetDisplayMode(1366);
	KMSDRM_SwapWindow();
	usleep(1000000);

	KMSDRM_SetDisplayMode(640);
	KMSDRM_SwapWindow();
	usleep(1000000);

	KMSDRM_SetDisplayMode(1366);
	KMSDRM_SwapWindow();
	usleep(1000000);

        KMSDRM_DestroyWindow();
	KMSDRM_VideoQuit();
	return 0;
}
