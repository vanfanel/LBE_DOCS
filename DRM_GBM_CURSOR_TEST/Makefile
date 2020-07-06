CFLAGS=-O0 -ggdb -I. -I/usr/include/drm -I/usr/include/GLES2
##CFLAGS=-O2 -I. -I/usr/include/drm
LDFLAGS=-lGLESv2 -lEGL -lm -ldrm -lgbm
TARGET=test
SOURCES=*.c
COMPILER=gcc

all:

	$(COMPILER) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)
clean:
	rm test
