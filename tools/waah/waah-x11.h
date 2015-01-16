#include <cairo-xlib.h>
#include <X11/Xlib.h>

typedef struct x11_app_s {
  app_t base;
} x11_app_t;

#define WAAH_APP_STRUCT x11_app_t
static x11_app_t *app;

static void
_app_run_xlib(mrb_state *mrb, mrb_value mrb_app, x11_app_t *x11_app) {
  app_t *app = (app_t *) x11_app;
  waah_canvas_t *canvas = (waah_canvas_t *) app;
  display_t *display = &app->display;
  keyboard_t *keyboard = app->keyboard;
  pointer_t *mouse = app->pointers[0];

  display->width = canvas->width;
  display->height = canvas->height;

  XSetLocaleModifiers("@im=none");

  Display *dpy = XOpenDisplay(NULL);
  if(dpy == NULL) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "could not open display");
    return;
  }

  Window w;

  int screen = DefaultScreen(dpy);
  w = XCreateSimpleWindow(dpy, RootWindow(dpy, screen),
                          1, 1, display->width, display->height, 0,
                          BlackPixel(dpy, screen),
                          BlackPixel(dpy, screen));

  XSelectInput(dpy, w, StructureNotifyMask |
                       ExposureMask |
                       KeyPressMask |
                       KeyReleaseMask |
                       ButtonPressMask  |
                       ButtonReleaseMask |
                       PointerMotionMask);

  XStoreName(dpy, w, app->title);

  XMapWindow(dpy, w);

  display->surface = cairo_xlib_surface_create(dpy, w,
                                               DefaultVisual(dpy, 0),
                                               display->width, display->height);


  XFlush(dpy);
  cairo_t *cr = cairo_create(display->surface);

  display->surface2 = cairo_surface_create_similar(display->surface, CAIRO_CONTENT_COLOR_ALPHA, display->width, display->height);

  int window_width = display->width, window_height = display->height;

  int fd = ConnectionNumber(dpy);
  fd_set fds;
  struct timeval tv;

  mrb_funcall(mrb, mrb_app, "setup", 0, NULL);

  if(mrb->exc) {
    mrb_print_error(mrb);
  }

  XIM im;
  XIC ic;
  im = XOpenIM(dpy, NULL, NULL, NULL);
  if(im == NULL) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "could not open input method");
    return;
  }

  ic = XCreateIC(im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
                 XNClientWindow, w, NULL);

  if(ic == NULL) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "could not open input controller");
    return;
  }

  XSetICFocus(ic);

  while(!app->quit) {
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    double secs = (1.0 / app->rate);
    tv.tv_sec = (time_t) (secs / 2.0);
    tv.tv_usec = (suseconds_t) (1000000 * (secs / 2.0 - (double)tv.tv_sec));

    if(select(fd + 1, &fds, 0, 0, &tv)) {
      app->redraw = TRUE;
      XEvent e;
      while(XPending(dpy)) XNextEvent(dpy, &e);

      if(XFilterEvent(&e, w)) {
        switch (e.type) {
          case MapNotify:
          case Expose:
            break;
          case ConfigureNotify: {
            XConfigureEvent *ev = (XConfigureEvent *) &e;
            if(ev->width != window_width ||
              ev->height != window_height) {

                window_width = ev->width;
                window_height = ev->height;

                cairo_surface_destroy(display->surface2);
                display->surface2 = cairo_surface_create_similar(display->surface, CAIRO_CONTENT_COLOR_ALPHA, window_width, window_height);
                cairo_xlib_surface_set_size(display->surface, window_width, window_height);
            }
            break;
          }
          case KeyPress: {
            XKeyPressedEvent *ev = (XKeyPressedEvent *) &e;
            app->time = ev->time;
            if(ev->keycode < N_KEYS) {
              keyboard->pressed[ev->keycode] = ev->time;
            }
            break;
          }
          case KeyRelease: {
            XKeyReleasedEvent *ev = (XKeyReleasedEvent *) &e;
            app->time = ev->time;
            if(ev->keycode < N_KEYS) {
              keyboard->released[ev->keycode] = ev->time;
            }
            break;
          }
          case MotionNotify: {
            XMotionEvent *ev = (XMotionEvent *) &e;
            app->time = ev->time;
            mouse->prev_x = mouse->x;
            mouse->prev_y = mouse->y;
            mouse->x = ev->x;
            mouse->y = ev->y;
            break;
          }
          case ButtonPress: {
            XButtonPressedEvent *ev = (XButtonPressedEvent *) &e;
            app->time = ev->time;
            mouse->x = ev->x;
            mouse->y = ev->y;
            if(ev->button < N_BUTTONS) {
              mouse->pressed[ev->button] = ev->time;
            }
            break;
          }
          case ButtonRelease: {
            XButtonReleasedEvent *ev = (XButtonReleasedEvent *) &e;
            app->time = ev->time;
            mouse->x = ev->x;
            mouse->y = ev->y;
            if(ev->button < N_BUTTONS) {
              mouse->released[ev->button] = ev->time;
            }
            break;
          }
          case KeymapNotify:
            XRefreshKeyboardMapping(&e.xmapping);
            break;
        }
      } else {
        switch(e.type) {
          case KeyPress: {
            if(!mrb_nil_p(keyboard->text_blk)) {
              int len;
              KeySym sym;
              char buffer[32];
              Status status;

              len = Xutf8LookupString(ic, (XKeyPressedEvent*)&e, buffer, 32, &sym, &status);

              if(status == XBufferOverflow) {
                // Overflow is unlikely, but could screw up string
                fprintf(stderr, "Received XBufferOverflow\n");
              } else {
                if(len > 0) {
                  printf("text: %.*s\n", len, buffer);
                }

                mrb_value mrb_str = mrb_str_new(mrb, buffer, len);
                mrb_funcall(mrb, keyboard->text_blk, "call", 1, mrb_str);
              }
            }
          }
        }
      }


    } 
    if(app->redraw) {
      gettimeofday(&tv, NULL);
      unsigned long long ts = tv.tv_sec * 1000000 + tv.tv_usec;
      if((double)(ts - app->last_redraw) / 1000000 > secs) {

        app->redraw = FALSE;
        app->last_redraw = ts;

        canvas->cr = cairo_create(display->surface2);
        mrb_funcall(mrb, mrb_app, "draw", 0, NULL);
        cairo_destroy(canvas->cr);

        cairo_set_source_surface(cr, display->surface2, 0, 0);
        cairo_paint(cr);

        XFlush(dpy);

      }
    }
  }

  cairo_destroy(cr);
	cairo_surface_destroy(display->surface);
  XCloseDisplay(dpy);
}
