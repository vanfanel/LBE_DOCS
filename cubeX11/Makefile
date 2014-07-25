CFLAGS=-O0 -ggdb -I. -I/usr/include/drm
##CFLAGS=-O2 -I. -I/usr/include/drm
LDFLAGS=-lGLESv2 -lEGL -lm -ldrm -lgbm -lX11
TARGET=cube
SOURCES=*.c
COMPILER=gcc

all:

	$(COMPILER) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)
clean:
	rm cube
