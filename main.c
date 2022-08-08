#include "main.h"

static int _XlibErrorHandler(Display *display, XErrorEvent *event) {
    fprintf(stderr, "An error occured detecting the mouse position\n");
    return True;
}

Display *display;

Window root_window;
Window window_returned;
Window window;
Window win_label;

unsigned int mask_return;

int screen;

XImage *image;

char msg[19];

int main(void) {
    Bool result;
    XColor color;
    XEvent event;

    setup_x();
    create_window();

    point root_pos;
    point win_pos;
    point prev_pos;

    while (True) {
        usleep(1000);
        XNextEvent(display, &event);

        switch (event.type) {
            case MotionNotify:
                break;

            case ButtonRelease:
                if (event.xbutton.button == 1) {
                    printf("rgb(%d, %d, %d)", color.red / 256, color.green / 256, color.blue / 256);
                    goto end;
                    // TODO: write to clipboard
                } else if (event.xbutton.button == 3) {
                    printf("right click");
                    goto end;
                }
                break;

            case Expose:
                render(root_pos, color);
                break;
        }

        if (query_pointer(&root_pos, &win_pos) != True) {
            fprintf(stderr, "No mouse found.\n");
            return -1;
        }

        // if the cursar hasnt moved then there is nothing to do
        if (root_pos.x == prev_pos.x && root_pos.y == prev_pos.y) {
            continue;
        }

        prev_pos = root_pos;

        pixel_color_at_pos(root_pos, &color);
        render(root_pos, color);
    }

end:
    XCloseDisplay(display);

    return 0;
}

int query_pointer(point *root_pos, point *win_pos) {
    return XQueryPointer(
        display,root_window,
        &window_returned, &window_returned,
        &root_pos->x, &root_pos->y,
        &win_pos->x, &win_pos->y,
        &mask_return
   );
}

void setup_x() {
    display = XOpenDisplay(NULL);
    assert(display);

    window = DefaultRootWindow(display);

    XSetErrorHandler(_XlibErrorHandler);
    root_window = XRootWindow(display, 0);
    XAllowEvents(display, AsyncBoth, CurrentTime);

    XGrabPointer(
        display,
        window,
        1,
        PointerMotionMask | ButtonReleaseMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None,
        CurrentTime
    );

    screen = DefaultScreen(display);
}

void create_window() {
    win_label = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        0, 0, 300, 100,
        0, BlackPixel(display, screen), WhitePixel(display, screen)
    );

    XMapWindow(display, win_label);
    XSelectInput(display, win_label, ExposureMask | KeyPressMask);

    XClassHint *hint = XAllocClassHint();
    hint->res_class = "floating";
    hint->res_name = "Color Picker";
    XSetClassHint(display, win_label, hint);
}

void pixel_color_at_pos(point pos, XColor *color) {
    image = XGetImage(
        display,
        XRootWindow(display, XDefaultScreen(display)),
        pos.x, pos.y,
        1, 1,
        AllPlanes,
        XYPixmap
    );

    color->pixel = XGetPixel(image, 0, 0);

    XFree(image);
    XQueryColor(
        display,
        XDefaultColormap(display, XDefaultScreen(display)),
        color
    );
}

void render(point pos, XColor color) {
    sprintf(msg, "rgb(%03d, %03d, %03d)", color.red / 256, color.green / 256, color.blue / 256);

    XMoveWindow(display, win_label, pos.x + 10, pos.y + 10);

    XClearWindow(display, win_label);
    XSetForeground(display, DefaultGC(display, screen), color.pixel);
    XFillRectangle(display, win_label, DefaultGC(display, screen), 20, 20, 50, 50);

    XSetForeground(display, DefaultGC(display, screen), BlackPixel(display, screen));
    XDrawString(display, win_label, DefaultGC(display, screen), 90, 50, msg, strlen(msg));
}
