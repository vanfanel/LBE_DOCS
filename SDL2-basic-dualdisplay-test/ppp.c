#include <SDL.h>
#include <stdio.h>

int src_width = 320;
int src_height = 200;

SDL_Window *window0 = NULL;                
SDL_Window *window1 = NULL;                
SDL_Renderer *renderer0 = NULL;
SDL_Renderer *renderer1 = NULL;
SDL_Texture *texture0 = NULL;
SDL_Texture *texture1 = NULL;
uint32_t *screen_pixels;

int windowTest ();

int main () {

    // Create the pixel array in memory
    screen_pixels = malloc(src_width * src_height * sizeof (uint32_t));
    for (int i = 0; i < (src_width * src_height); i++) 
    {
        screen_pixels[i] = 0xffff0000;
    }    

    SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2

    windowTest();
   
    // Clean up
    SDL_Quit();
    return 0;
}

// Create Window and renderer
int windowTest () {

    int i, x, y;
    SDL_Rect displayBounds[2];

    int num_displays = SDL_GetNumVideoDisplays();


    /* MULTIPLE DISPLAYS TESTING BLOCK */

    printf("===AVAILABLE DISPLAYS %d===\n", num_displays);

    for (i = 0; i < num_displays; i++) {
        SDL_GetDisplayBounds(i, &displayBounds[i]);
        printf ("DISPLAY %d: WIDTH %d HEIGHT %d xpos %d ypos %d\n",
            i, displayBounds[i].w, displayBounds[i].h, displayBounds[i].x, displayBounds[i].y);
    }

#if 1
    x = displayBounds[0].x;
    y = displayBounds[0].y;

    // Create an application window with the following settings:
    window0 = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        x,           // initial x position
        y,           // initial y position
        320,                               // width, in pixels
        200,                               // height, in pixels
        0      // flags - see below
    );
#endif

#if 1
    x = displayBounds[1].x;
    y = displayBounds[1].y;

    // Create an application window with the following settings:
    window1 = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        x,           // initial x position
        y,           // initial y position
        320,                               // width, in pixels
        200,                               // height, in pixels
        0      // flags - see below
    );
#endif

    /* MULTIPLE DISPLAYS TESTING BLOCK ENDS */

    renderer0 = SDL_CreateRenderer(window0, -1, SDL_RENDERER_ACCELERATED);
    renderer1 = SDL_CreateRenderer(window1, -1, SDL_RENDERER_ACCELERATED);

#if 1
    // Create texture for renderer 0
    texture0 = SDL_CreateTexture(renderer0,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               src_width, src_height);
#endif

#if 1
    // Create texture for renderer 1
    texture1 = SDL_CreateTexture(renderer1,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               src_width, src_height);
#endif

    // Present the image
    
    // Dump pixel array to texture
    SDL_UpdateTexture(texture0, NULL, screen_pixels, src_width * sizeof (Uint32));
    SDL_UpdateTexture(texture1, NULL, screen_pixels, src_width * sizeof (Uint32));

    /* Copy texture to renderer 0. */
    SDL_RenderClear(renderer0);
    SDL_RenderCopy(renderer0, texture0, NULL, NULL);

#if 1
    /* Copy texture to renderer 1. */
    SDL_RenderClear(renderer1);
    SDL_RenderCopy(renderer1, texture1, NULL, NULL);
#endif

    /* Present renderer 0 contents on screen */
    SDL_RenderPresent(renderer0);

#if 1
    /* Present renderer 1 contents on screen */
    SDL_RenderPresent(renderer1);
#endif

    // Pause execution for n milliseconds
    SDL_Delay(4000);  

    // Destroy texture, renderer and window
    SDL_DestroyTexture(texture0);
    SDL_DestroyTexture(texture1);

    //SDL_DestroyRenderer(renderer0);
#if 1
    SDL_DestroyRenderer(renderer1);
#endif

    //SDL_DestroyWindow(window0);

#if 1
    SDL_DestroyWindow(window1);
#endif
}
