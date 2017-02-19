CFLAGS=-O0 -ggdb -I. -I/usr/include/drm
##CFLAGS=-O2 -I. -I/usr/include/drm
LDFLAGS=-lm -ldrm
TARGET=test
SOURCES=*.c
COMPILER=gcc

all:

	$(COMPILER) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)
clean:
	rm test
