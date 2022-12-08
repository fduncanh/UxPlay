/**
 * RPiPlay - An open-source AirPlay mirroring server for Raspberry Pi
 * Copyright (C) 2019 Florian Draschbacher
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

/* based on code from David Ventura https://github.com/DavidVentura/UxPlay */

#ifndef X_DISPLAY_FIX_H
#define X_DISPLAY_FIX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>

struct X11_Window_s {
    Display * display;
    Window window;
} typedef X11_Window_t;

void get_X11_Display(X11_Window_t * X11) {
    X11->display = XOpenDisplay(NULL);
    X11->window = (Window) NULL;
}

Window enum_windows(const char * str, Display * display, Window window, int depth) {
    int i;
    XTextProperty text;
    XGetWMName(display, window, &text);
    char* name;
    XFetchName(display, window, &name);
    if (name != 0 &&  strcmp(str, name) == 0) {
        return window;
    }
    Window _root, parent;
    Window* children;
    unsigned int n;
    XQueryTree(display, window, &_root, &parent, &children, &n);
    if (children != NULL) {
        for (i = 0; i < n; i++) {
            Window w = enum_windows(str, display, children[i], depth + 1);
            if (w) return w;
        }
        XFree(children);
    }
    return (Window) NULL;
}

// Fullscreen mod


void set_fullscreen(Display* dpy, Window win, const char * name, bool* fullscreen)
{
    // *fullscreen = !(*fullscreen);    
    XClientMessageEvent msg = {
        .type = ClientMessage,
        .display = dpy,
        .window = win,
        .message_type = XInternAtom(dpy, "_NET_WM_STATE", True),
        .format = 32,
        .data = { .l = {
                *fullscreen,
                XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", True),
                None,
                0,
                1
            }}
    };
    Window root = XDefaultRootWindow(dpy);     
    win  = enum_windows(name, dpy, root, 0);
    if (win) {
        XSendEvent(dpy, XRootWindow(dpy, XDefaultScreen(dpy)), False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent*) &msg);
        XSync(dpy, False);
    }
}

void fix_x_window_name(X11_Window_t * X11, const char * name) {
    Window root = XDefaultRootWindow(X11->display);     
    X11->window  = enum_windows(name, X11->display, root, 0);
    if (X11->window) {
        Atom _NET_WM_NAME = XInternAtom(X11->display, "_NET_WM_NAME", 0);
        Atom UTF8_STRING = XInternAtom(X11->display, "UTF8_STRING", 0);
        XChangeProperty(X11->display, X11->window, _NET_WM_NAME, UTF8_STRING, 
                        8, 0, (const unsigned char *) name, strlen(name));
        XSync(X11->display, False);       
    }
}

#ifdef __cplusplus
}
#endif

#endif 
