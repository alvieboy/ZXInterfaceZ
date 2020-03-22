CFLAGS=-O2 -g $(shell pkg-config --cflags sdl2 xrandr)
LIBS=$(shell pkg-config --libs sdl2 xrandr)


all: viewer

viewer: viewer.o
	$(CC) -o $@ $< $(LIBS)
