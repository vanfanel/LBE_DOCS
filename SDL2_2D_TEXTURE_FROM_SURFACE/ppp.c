#include <SDL.h>
#include <stdio.h>

#define WINDOW_WIDTH 1280  
#define WINDOW_HEIGHT 800
#define TEXTURE_WIDTH 640
#define TEXTURE_HEIGHT 400
#define GAME_FRAME_TIME 130
#define SCROLL_FRAME_TIME 10

int minimumFrameTime = 130;

SDL_Window *window = NULL;                
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
SDL_Surface *surface = NULL;
uint32_t *screen_pixels;

void init ();
void deinit ();
void main_loop();
void render();
void clear_surface();
void handle_events();

int main () {

    init();

    main_loop();

    deinit();
   
    SDL_Quit();
    return 0;
}

// Create Window and renderer
void init () {

    const Uint32 rmask = 0x000000ff;
    const Uint32 gmask = 0x0000ff00;
    const Uint32 bmask = 0x00ff0000;
    const Uint32 amask = 0xff000000;

    SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_ShowCursor(SDL_FALSE);

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        WINDOW_WIDTH,                      // width, in pixels
        WINDOW_HEIGHT,                     // height, in pixels
        //SDL_WINDOW_FULLSCREEN_DESKTOP    // flags - see below
        0
    );

    // Check that the window was successfully created
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    //SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

    surface = SDL_CreateRGBSurface(0, TEXTURE_WIDTH, TEXTURE_HEIGHT, 32, rmask, gmask, bmask, amask);

    // Create texture
    /*texture = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               src_width, src_height);
    */
    texture = SDL_CreateTextureFromSurface(renderer, surface);	

    // Create the pixel array in memory
    // screen_pixels = malloc(src_width * src_height * sizeof (uint32_t));
}

void deinit() {
    // Destroy texture, surface, renderer and window
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void main_loop() {

    // Update pixel surface contentx and render.

    int i, j, k, m;

    SDL_PixelFormat *format = surface->format;
    Uint8 bytes_per_pixel = format->BytesPerPixel;
 
    //clear_surface ();
    //render();

    for (j = 0; j < TEXTURE_WIDTH - 20; j++) {
           
            unsigned int frameTime = SDL_GetTicks();

            clear_surface ();
            
            for (i = 0; i < TEXTURE_HEIGHT; i++) {
                    
                    for (k = 0; k < 20; k++) { 

                            *((uint32_t*) surface->pixels + (i * surface->w + j + k)) = 0xffff0000;
                                                                                        //AABBGGRR	
                    }
            }
   
            handle_events(); 
            render();

            //cap the frame rate (comment out for full framerate operation)
            if (SDL_GetTicks() - frameTime < 10){
                SDL_Delay(10 - (SDL_GetTicks() - frameTime));
            }
    }
}

void render () {
    // Dump pixel array to texture
    // SDL_UpdateTexture(texture, NULL, screen_pixels, src_width * sizeof (Uint32));

    // Dump surface pixels to texture
    SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);

    // Rendercopy the texture to the renderer
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    // Renderpresent the renderer
    SDL_RenderPresent(renderer);

    //SDL_Delay(2000);
}

void clear_surface () {
	// Clear screen
	
	int i;
	for (i = 0; i < surface->w * surface->h; i++) {
		((uint32_t *)surface->pixels)[i] = 0x00000000;
        }
}

void handle_events()
{
    SDL_Event event;	
    while(SDL_PollEvent(&event))
    {


    } 
}
