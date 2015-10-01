#include <stdbool.h>

// Generic GLES2 fuctions that should work on every GLES2 platform

void gles2_init(int texture_width, int texture_height, int depth, bool maintain_aspect_ratio);
void gles2_draw(void *);
