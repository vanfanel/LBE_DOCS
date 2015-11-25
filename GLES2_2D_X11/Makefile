CFLAGS=-O0 -ggdb -I.
##CFLAGS=-O2 -I.
LDFLAGS=-lGLESv2 -lEGL -lm -lX11
TARGET=test
SOURCES=*.c
COMPILER=gcc

all:

	$(COMPILER) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)
clean:
	rm test
