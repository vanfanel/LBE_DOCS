#include <SDL.h>
#include <stdio.h>

int CreateWindow (SDL_Window **window);

int main () {

    SDL_Window *window = NULL;                
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;

    // Create the pixel array in memory
    int src_width = 320;
    int src_height = 200;

    uint32_t *screen_pixels = malloc(src_width * src_height * sizeof (uint32_t));
    for (int i = 0; i < (src_width * src_height); i++) 
    {
        screen_pixels[i] = 0xffff0000;
    }    

    SDL_Init(SDL_INIT_EVERYTHING);              // Initialize SDL2

    // PART I

    // Create Window and renderer
    CreateWindow(&window);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

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





    // PART II

    SDL_Delay(500);  



    // Create Window and renderer
    CreateWindow(&window);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

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

    // Clean up
    SDL_Quit();
    return 0;
}

// Create Window and renderer
int CreateWindow (SDL_Window **window) {

    // Create an application window with the following settings:
    *window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        640,                               // width, in pixels
        480,                               // height, in pixels
        SDL_WINDOW_FULLSCREEN_DESKTOP                  // flags - see below
    );

    // Check that the window was successfully created
    if (*window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }
}
