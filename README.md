Basic examples and functions intended for easy smooth 2D graphics 
(multiple buffers, vsync) using GLES2 for scaling and simple 
post processing effects with fragment shaders.

GLES2 is used for scaling, instead of using the 2D API, to avoid shader "distortion":
for example we can't use the 2D API to scale after we applied a scanline shader on the 
game image texture or we will get thick deformed scanlines.

GLES2 runs on several underlying 2D APIs like DRM/KMS, Dispmanx (Raspberry PI)
and of course the slow X11, so these functions allow to easily deploy 2D stuff
like games or emulators with smooth movement on almost any GNU/Linux-based platform.
GLES2-X11: the example GLES2-2D program running on X11. 
GLES2-KMS: the example GLES2-2D program running on KMS. 
GLES2-DISPMANX: the example GLES2-2D program running on the native Raspberry Pi 2D API. 
