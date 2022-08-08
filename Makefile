default: build

build:
	gcc main.c -L/usr/X11R6/lib -lX11 -o color_picker


