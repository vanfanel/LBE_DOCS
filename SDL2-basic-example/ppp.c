#include <SDL2/SDL.h>
#include <stdio.h>

int src_width = 320;
int src_height = 200;

SDL_Window *window = NULL;                
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
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

    int i;
    SDL_Rect displayBounds[2];

    int num_displays = SDL_GetNumVideoDisplays();


    /* MULTIPLE DISPLAYS TESTING BLOCK */
/*
    printf("===AVAILABLE DISPLAYS %d===\n", num_displays);

    for (i = 0; i < num_displays; i++) {
        SDL_GetDisplayBounds(i, &displayBounds[i]);
        printf ("DISPLAY %d: WIDTH %d HEIGHT %d xpos %d ypos %d\n",
            i, displayBounds[i].w, displayBounds[i].h, displayBounds[i].x, displayBounds[i].y);
    }


    int x = displayBounds[0].x;
    int y = displayBounds[0].y;

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        x,           // initial x position
        y,           // initial y position
        320,                               // width, in pixels
        200,                               // height, in pixels
        0      // flags - see below
    );

    x = displayBounds[1].x;
    y = displayBounds[1].y;

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        x,           // initial x position
        y,           // initial y position
        320,                               // width, in pixels
        200,                               // height, in pixels
        0      // flags - see below
    );
*/
    /* MULTIPLE DISPLAYS TESTING BLOCK ENDS */

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        320,                               // width, in pixels
        200,                               // height, in pixels
        SDL_WINDOW_FULLSCREEN_DESKTOP      // flags - see below
    );

    // Check that the window was successfully created
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Aspect rate correction
    SDL_RenderSetLogicalSize(renderer, src_width, src_height);

    // Create texture
    texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               src_width, src_height);

    // Present the image
    
    // Dump pixel array to texture
    SDL_UpdateTexture(texture, NULL, screen_pixels, src_width * sizeof (Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    // Pause execution for n milliseconds
    SDL_Delay(2000);  

    // Destroy texture, renderer and window
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}
