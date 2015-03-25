#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <cairo.h>

typedef struct linuxfb_app_s {
  app_t base;
	int fb_fd;
	unsigned char *fb_data;
	long fb_screensize;
	struct fb_var_screeninfo fb_vinfo;
	struct fb_fix_screeninfo fb_finfo;
} linuxfb_app_t;

#define WAAH_APP_STRUCT linuxfb_app_t
static linuxfb_app_t *app;

static void
_app_run_linuxfb(mrb_state *mrb, mrb_value mrb_app, linuxfb_app_t *linuxfb_app) {
  app_t *app = (app_t *) linuxfb_app;
  waah_canvas_t *canvas = (waah_canvas_t *) app;
  display_t *display = &app->display;
  char *fb_name = NULL;


  // Loosley based on
  // http://lists.cairographics.org/archives/cairo/2010-July/020378.html
  // proposed for inclusion in cairo
  // thus license assumed to be at least compatible with MPL

	if (fb_name == NULL) {
		fb_name = "/dev/fb0";
	}

	linuxfb_app->fb_fd = open(fb_name, O_RDWR);
	if (linuxfb_app->fb_fd == -1) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "could not open framebuffer device");
    return;
	}

	if (ioctl(linuxfb_app->fb_fd, FBIOGET_VSCREENINFO, &linuxfb_app->fb_vinfo) == -1) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "could not obtain screen info");
    return;
	}

  display->width = canvas->width = linuxfb_app->fb_vinfo.xres;
  display->height = canvas->height = linuxfb_app->fb_vinfo.yres;

	linuxfb_app->fb_screensize = linuxfb_app->fb_vinfo.xres * linuxfb_app->fb_vinfo.yres
	                        * linuxfb_app->fb_vinfo.bits_per_pixel / 8;

	linuxfb_app->fb_data = (char *)mmap(0, linuxfb_app->fb_screensize,
	                               PROT_READ | PROT_WRITE, MAP_SHARED,
	                               linuxfb_app->fb_fd, 0);
	if ((int)linuxfb_app->fb_data == -1) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "mmap failed");
    return;
	}

	if (ioctl(linuxfb_app->fb_fd, FBIOGET_FSCREENINFO, &linuxfb_app->fb_finfo) == -1) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "could not obtain screen info");
    return;
	}

  cairo_format_t cr_format;
  switch(linuxfb_app->fb_vinfo.bits_per_pixel) {
    case 16: 
      cr_format = CAIRO_FORMAT_RGB16_565;
      break;
    case 32:
      cr_format = CAIRO_FORMAT_ARGB32;
      break;
    default:
      mrb_raise(mrb, E_RUNTIME_ERROR, "invalid framebuffer depth");
      return;
  }

  fprintf(stderr, "bpp %d\n", linuxfb_app->fb_vinfo.bits_per_pixel);

	display->surface = cairo_image_surface_create_for_data(linuxfb_app->fb_data,
                       cr_format,
                       linuxfb_app->fb_vinfo.xres,
                       linuxfb_app->fb_vinfo.yres,
                       cairo_format_stride_for_width(cr_format,
                       linuxfb_app->fb_vinfo.xres));


  cairo_t *cr = cairo_create(display->surface);

  display->surface2 = cairo_surface_create_similar(display->surface, CAIRO_CONTENT_COLOR_ALPHA, display->width, display->height);
  mrb_funcall(mrb, mrb_app, "setup", 0, NULL);

  if(mrb->exc) {
    mrb_print_error(mrb);
  }

  struct timeval tv;
  while(!app->quit) {
    unsigned long long usecs = (1.0 / (double)app->rate) * 1000000;

    if(app->redraw) {
      gettimeofday(&tv, NULL);
      unsigned long long ts = tv.tv_sec * 1000000 + tv.tv_usec;
      if(ts - app->last_redraw > usecs) {
        app->redraw = FALSE;
        app->last_redraw = ts;

        canvas->cr = cairo_create(display->surface2);
        mrb_funcall(mrb, mrb_app, "draw", 0, NULL);
        cairo_destroy(canvas->cr);
        canvas->cr = NULL;

        ++app->time;

        cairo_set_source_surface(cr, display->surface2, 0, 0);
        cairo_paint(cr);
      }
    }
    usleep(usecs / 2);

  }

  cairo_destroy(cr);
	cairo_surface_destroy(display->surface);
	cairo_surface_destroy(display->surface2);
}
