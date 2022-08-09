#include "clipboard.h"

Atom targets_atom;
Atom text_atom;
Atom UTF8;
Atom selection;

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
    printf("getting selection ownership\n");
    XSetSelectionOwner(display, selection, window, CurrentTime);
    owner = XGetSelectionOwner(display, selection);

    if (owner != window) {
        printf("failed to get selection ownership");
        return;
    }

    // fork background process to handle requests
    /* printf("forking"); */
    /* pid_t pid = fork(); */
    /* if (pid) { */
    /*     printf("killing parent"); */
    /*     XSetSelectionOwner(display, selection, None, CurrentTime); */
    /*     XUngrabPointer(display, CurrentTime); */
    /*     exit(EXIT_SUCCESS); */
    /* } */

    printf("ungrab\n");
    XUngrabPointer(display, CurrentTime);

    while (True) {
        printf("looping\n");
        usleep(1000);
        XNextEvent(display, &event);
        printf("got event");

        switch (event.type) {
            case SelectionClear:
                printf("clear\n");
                return;

            case SelectionRequest:
                printf("request\n");
                if (event.xselectionrequest.selection != selection) {
                    printf("wrong selection\n");
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
                int R = 0;

                if (xev.target == targets_atom) {
                    R = XChangeProperty(xev.display, xev.requestor, xev.property, XA_ATOM, 32, PropModeReplace, (unsigned char*)&UTF8, 1);
                } else if (xev.target == XA_STRING || xev.target == text_atom) {
                    R = XChangeProperty(xev.display, xev.requestor, xev.property, XA_STRING, 8, PropModeReplace, text, length);
                } else if (xev.target == UTF8) {
                    R = XChangeProperty(xev.display, xev.requestor, xev.property, UTF8, 8, PropModeReplace, text, length);
                } else {
                    xev.property = None;
                }

                if ((R & 2) == 0) {
                    printf("sending: %s\n", text);
                    XSendEvent(display, xev.requestor, 0, 0, (XEvent *)&xev);
                    printf("sent\n");
                }
            break;
        }
    }

    XSetSelectionOwner(display, selection, None, CurrentTime);
}
