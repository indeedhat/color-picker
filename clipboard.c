#include "clipboard.h"

#define FORK 1

static Atom targets_atom;
static Atom text_atom;
static Atom UTF8;
static Atom selection;

void init_clipboard(Display *display) {
    targets_atom = XInternAtom(display, "TARGETS", 0);
    text_atom = XInternAtom(display, "TEXT", 0);
    UTF8 = XInternAtom(display, "UTF8", 0);

    if (UTF8 == None) {
        UTF8 = XA_STRING;
    }

    selection = XInternAtom(display, "CLIPBOARD", 0);
}

void copy_to_clipboard(Display *display, Window window, unsigned char *text, int length) {
    XEvent event;
    Window owner;

    // take ownership of selection
    XSetSelectionOwner(display, selection, window, CurrentTime);
    owner = XGetSelectionOwner(display, selection);

    if (owner != window) {
        printf("failed to get selection ownership");
        return;
    }

#ifdef FORK
    // fork background process to handle requests
    pid_t pid = fork();
    if (pid) {
        XSetSelectionOwner(display, selection, None, CurrentTime);
        XUngrabPointer(display, CurrentTime);
        exit(EXIT_SUCCESS);
    }
#endif

    XUngrabPointer(display, CurrentTime);

    while (True) {
        usleep(1000);
        XNextEvent(display, &event);

        switch (event.type) {
            case SelectionClear:
                return;

            case SelectionRequest:
                if (event.xselectionrequest.selection != selection) {
                    break;
                }

                XSelectionRequestEvent *xreq = &event.xselectionrequest;
                XSelectionEvent xev = {0};

                xev.type = SelectionNotify;
                xev.display = xreq->display;
                xev.requestor = xreq->requestor;
                xev.selection = xreq->selection;
                xev.time = xreq->time;
                xev.target = xreq->target;
                xev.property = xreq->property;

                // dont really know what this does
                int response = 0;

                if (xev.target == targets_atom) {
                    response = XChangeProperty(xev.display, xev.requestor, xev.property, XA_ATOM, 32, PropModeReplace, (unsigned char*)&UTF8, 1);
                } else if (xev.target == XA_STRING || xev.target == text_atom) {
                    response = XChangeProperty(xev.display, xev.requestor, xev.property, XA_STRING, 8, PropModeReplace, text, length);
                } else if (xev.target == UTF8) {
                    response = XChangeProperty(xev.display, xev.requestor, xev.property, UTF8, 8, PropModeReplace, text, length);
                } else {
                    xev.property = None;
                }

                if ((response & BadValue) == 0) {
                    XSendEvent(display, xev.requestor, 0, 0, (XEvent *)&xev);
                }
            break;
        }
    }

    XSetSelectionOwner(display, selection, None, CurrentTime);
}
