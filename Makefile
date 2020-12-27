all: niceport

niceport:
	gcc nice.c util.c callbacks.c niceport.c -g `pkg-config --cflags --libs nice` -o bin/niceport_raw
