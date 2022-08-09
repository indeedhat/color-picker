#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void init_clipboard(Display *display);

void copy_to_clipboard(Display *display, Window window, unsigned char *text, int length);
