#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int x;
    int y;
} point;


int query_pointer(point *, point *);
void pixel_color_at_pos(point, XColor *);

void setup_x();
void create_window();

void render(point, XColor);

