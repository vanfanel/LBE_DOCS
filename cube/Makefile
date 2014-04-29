CFLAGS=-O0 -ggdb -I. -I/usr/include/drm
LDFLAGS=-lGLESv2 -lEGL -lm -ldrm -lgbm
TARGET=triangle
SOURCES=*.c
COMPILER=gcc

all:

	$(COMPILER) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)
clean:
	rm triangle
