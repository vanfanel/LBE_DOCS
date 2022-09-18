#include <SDL2/SDL.h>
#include <stdio.h>

int src_width = 320;
int src_height = 200;

SDL_Window *window = NULL;                
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
uint32_t *screen_pixels;

SDL_bool end_test = SDL_FALSE;



void init_video () {
    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        src_width,                         // width, in pixels
        src_height,                        // height, in pixels
        SDL_WINDOW_FULLSCREEN_DESKTOP      // flags - see below
    );

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Aspect rate correction
    SDL_RenderSetLogicalSize(renderer, src_width, src_height);

    // Mouse confination rect test.
    SDL_Rect mrect;
    mrect.w = 80;
    mrect.h = 80;
    mrect.x = 0;
    mrect.y = 0;

    SDL_SetWindowMouseRect(window, &mrect);

    // Create texture
    texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               src_width, src_height);
}

void deinit_video () {
    // Destroy texture, renderer and window
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

// Create Window and renderer
int render_test () {
    
    // Present the image
    
    // Dump pixel array to texture
    SDL_UpdateTexture(texture, NULL, screen_pixels, src_width * sizeof (Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void read_input () {

    // Event handler
    SDL_Event event;

    // Handle events on queue
    while (SDL_PollEvent(&event) != 0) {
        // User requests quit
        if (event.type == SDL_QUIT)
        {
            end_test = SDL_TRUE;
        }	
        // User presses a key
        else if (event.type == SDL_KEYDOWN)
        {
            switch (event.key.keysym.sym)
            {
                case SDLK_ESCAPE:
                end_test = SDL_TRUE;
                break;
            }
        }   
    }   
}

int main () {

    // Create the pixel array in memory
    screen_pixels = malloc(src_width * src_height * sizeof (uint32_t));
    for (int i = 0; i < (src_width * src_height); i++) 
    {
        screen_pixels[i] = 0xffff0000;
    }    

    // Initialize SDL2
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS);              

    init_video();

    // main loop
    while (!end_test) {
        render_test();
        read_input();
    }
    ////////////

    deinit_video();

    // Clean up
    SDL_Quit();
    return 0;
}
