#include "main.h"
#include "clipboard.h"


static int _XlibErrorHandler(Display *display, XErrorEvent *event) {
    fprintf(stderr, "An error occured detecting the mouse position\n");
    return True;
}

static Display *display;
static Window root_window;
static Window window_returned;
static Window window;
static Window win_label;
static unsigned int mask_return;
static int screen;
static XImage *image;
static char *rgbText;
static int display_mode;

int main(void) {
    Bool result;
    XColor color;
    XEvent event;
    point root_pos;
    point win_pos;
    point prev_pos;
    rgbText = malloc(19);

    setup_x();
    create_window();
    init_clipboard(display);

    unsigned int left_alt = XKeysymToKeycode(display, XK_Alt_L);
    unsigned int right_alt = XKeysymToKeycode(display, XK_Alt_R);

    int force_render = 0;

    while (True) {
        usleep(1000);
        XNextEvent(display, &event);

        switch (event.type) {
            case MotionNotify:
                break;

            case ButtonRelease:
                if (event.xbutton.button == 1) {
                    goto clipboard;
                } else if (event.xbutton.button == 3) {
                    goto end;
                }
                break;

            case Expose:
                render(root_pos, color);
                break;

            case KeyPress:
                if (event.xkey.keycode == left_alt || event.xkey.keycode == right_alt) {
                    display_mode = ((display_mode + 1) % DisplayModeCount);
                    force_render = 1;
                }
                break;
        }

        if (query_pointer(&root_pos, &win_pos) != True) {
            fprintf(stderr, "No mouse found.\n");
            return -1;
        }

        // if the cursar hasnt moved then there is nothing to do
        if (!force_render && root_pos.x == prev_pos.x && root_pos.y == prev_pos.y) {
            continue;
        }

        prev_pos = root_pos;

        pixel_color_at_pos(root_pos, &color);
        render(root_pos, color);
        force_render = 0;
    }

clipboard:
    // close the color picker window
    XUnmapWindow(display, win_label);
    // copy to clipboard and fork the process to listen for requests
    // child process will be closed when another program takes control of the selection
    copy_to_clipboard(display, window, (unsigned char *)rgbText, strlen(rgbText));
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

    XSelectInput(display, window, KeyPressMask);
    screen = DefaultScreen(display);
}

void create_window() {
    win_label = XCreateSimpleWindow(
        display,
        RootWindow(display, screen),
        0, 0, 180, 60,
        0, BlackPixel(display, screen), WhitePixel(display, screen)
    );

    char *window_name = "Color Picker";
    Atom properties[3];
    Atom utf8_str = XInternAtom(display, "UTF8_STRING", False);

    XStoreName(display, win_label, window_name);
    properties[2] = XInternAtom(display, "_NET_WM_NAME", False);
    XChangeProperty(display, win_label, properties[2], utf8_str, 8, PropModeReplace, (unsigned char *)window_name, 12);

    XClassHint hint = { window_name, window_name };
    XSetClassHint(display, win_label, &hint);

    // set as utility window (floatin in i3)
    properties[0] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NOTIFICATION", False);
    properties[1] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", False);
    properties[2] = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    XChangeProperty(display, win_label, properties[2], XA_ATOM, 32, PropModeReplace, (unsigned char *) properties, 2L);

    properties[0] = XInternAtom(display, "_NET_WM_STATE_ABOVE", False);
    properties[2] = XInternAtom(display, "_NET_WM_STATE", False);
    XChangeProperty(display, win_label, properties[2], XA_ATOM, 32, PropModeReplace, (unsigned char *) properties, 1L);

    XMapWindow(display, win_label);
    XSelectInput(display, win_label, ExposureMask | KeyPressMask);
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
    switch (display_mode) {
        case RGBMode:
            snprintf(rgbText, 19, "rgb(%d, %d, %d)", color.red / 256, color.green / 256, color.blue / 256);
            break;
        case HexMode:
            snprintf(rgbText, 7, "#%02x%02x%02x", color.red / 256, color.green / 256, color.blue / 256);
            break;
        case RGBValueMade:
            snprintf(rgbText, 19, "%d, %d, %d", color.red / 256, color.green / 256, color.blue / 256);
            break;
    }

    XMoveWindow(display, win_label, pos.x + 10, pos.y + 10);

    XClearWindow(display, win_label);
    XSetForeground(display, DefaultGC(display, screen), color.pixel);
    XFillRectangle(display, win_label, DefaultGC(display, screen), 5, 5, 50, 50);

    XSetForeground(display, DefaultGC(display, screen), BlackPixel(display, screen));
    XDrawRectangle(display, win_label, DefaultGC(display, screen), 5, 5, 50, 50);
    XDrawString(display, win_label, DefaultGC(display, screen), 65, 35, rgbText, strlen(rgbText));
}
