CFLAGS=-O0 -DEGL_NO_X11 -ggdb -I. -I/usr/include/libdrm -I/usr/local/include/libdrm -I/usr/local/include -I/usr/local/include/GLES2
##CFLAGS=-O2 -I. -I/usr/include/drm
LDFLAGS=-lGLESv2 -lEGL -lm -ldrm -lgbm
TARGET=test
SOURCES=*.c
COMPILER=gcc

all:

	$(COMPILER) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)
clean:
	rm test
