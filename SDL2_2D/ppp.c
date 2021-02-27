#include <SDL.h>
#include <stdio.h>

int src_width = 320;
int src_height = 200;

SDL_Window *window = NULL;                
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
uint32_t *screen_pixels;

void init ();
void deinit ();
void main_loop();
void render();
void clear_screen (int width, int height, void* pixels);

int main () {

    init();

    main_loop();

    deinit();
   
    SDL_Quit();
    return 0;
}

// Create Window and renderer
void init () {

    SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        320,                               // width, in pixels
        200,                               // height, in pixels
        //SDL_WINDOW_FULLSCREEN_DESKTOP      // flags - see below
        0
    );

    // Check that the window was successfully created
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Create texture
    texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               src_width, src_height);

    // Create the pixel array in memory
    screen_pixels = malloc(src_width * src_height * sizeof (uint32_t));
}

void deinit() {
    // Destroy texture, renderer and window
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void main_loop() {
    // Update pixel array contents

    int i, j, k, m;
    int total_pitch = 320 * 4; /*4 bpp*/
    
    clear_screen (320, 200,  screen_pixels);

    int ret;	
    
    for (m = 0; m < 1; m++) {
            for (j = 0; j < 320 - 20; j++) {
                    
                    clear_screen (320, 200,  screen_pixels);
                    
                    for (i = 0; i < 200; i++) {
                            
                            for (k = 0; k < 20; k++) { 
                                    //screen_pixels[i * 320 + j + k] = 0x0FF0;
                                                              //AABBGGRR	
                                    screen_pixels[i * 320 + j + k] = 0x000000FF;
                            }
                    }
            
                    render();
        
            }
    }


/*
    for (int i = 0; i < (src_width * src_height); i++) 
    {
        screen_pixels[i] = 0xffff0000;
    }    
*/

}

void render () {
    // Dump pixel array to texture
    SDL_UpdateTexture(texture, NULL, screen_pixels, src_width * sizeof (Uint32));

    // Rendercopy the texture to the renderer
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    // Renderpresent the renderer
    SDL_RenderPresent(renderer);

    //SDL_Delay(2000);
}

void clear_screen (int width, int height, void* pixels) {
	// Clear screen
	
	int i;
	for (i = 0; i < width * height ; i++)
		((uint32_t *)pixels)[i] = 0x00000000;
}
