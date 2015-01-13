#include <windows.h>
#include <cairo/cairo-win32.h>
#include <shellapi.h>
//
// Roughly based on w32dry which was relased in the public domain
// http://canonical.org/~kragen/sw/w32dry/

typedef struct windows_app_s {
  app_t base;
  HINSTANCE hInstance;
  int nCmdShow;
  mrb_state *mrb;
} windows_app_t;

#define WAAH_APP_STRUCT windows_app_t
static windows_app_t *app;

typedef struct {
  PWNDCLASSEX class;
  HWND hWnd;
  char *title;
  int style;
  int x, y;
} window_instance;

// call this to initialize a WNDCLASSEX and a window_instance.
// doesn't register the window class.  do that yourself.
void init_window_classex(PWNDCLASSEX class,    // pointer to uninitialized class
                         window_instance *win, // pointer to uninitialized inst
                         HINSTANCE hInstance,  // of the application
                         WNDPROC WndProc,
                         LPCSTR name);         // of the window class

// call this to actually instantiate a window handle
// returns 0 on failure
int create_window(window_instance *win, int width, int height);
// call this to pump messages through to your windows
// returns an exit code for the app (normally 0)
int message_loop();



// for things that you want to MessageBox about and exit if they fail
#define check(foo) pcheck(foo, int)
#define pcheck(foo, type) (type)_check(#foo, (int)foo)
int _check(char *contents, int result);


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
  waah_canvas_t *canvas = (waah_canvas_t *) app;
  switch(msg) {
  case WM_MOUSEMOVE: 
    {
      HDC hDC = GetDC(hWnd);
      //draw_text(hDC, LOWORD(lParam), HIWORD(lParam));
      ReleaseDC(hWnd, hDC);
      break;
    }
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  case WM_PAINT: 
    {
      PAINTSTRUCT paintStruct;
      HDC hDC = BeginPaint(hWnd, &paintStruct);
      canvas->surface = cairo_win32_surface_create(hDC);
      canvas->cr = cairo_create(canvas->surface);
      mrb_funcall(app->mrb, mrb_app, "draw", 0, NULL);
      cairo_destroy(canvas->cr);
      cairo_surface_destroy(canvas->surface);
      EndPaint(hWnd, &paintStruct);
      break;
    }
  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

static void
_app_run_windows(mrb_state *mrb, mrb_value mrb_app, windows_app_t *windows_app) {
  yeah_canvas_t *canvas = (yeah_canvas_t *) windows_app;
  WNDCLASSEX windowClass;
  window_instance win;
  init_window_classex(&windowClass, &win, windows_app->hInstance, WndProc, "AClass");
  if (!RegisterClassEx(&windowClass)) return;
  win.title = "Waah";
  if (!create_window(&win, canvas->width, canvas->height)) return;
  ShowWindow(win.hWnd, windows_app->nCmdShow);

  windows_app->mrb = mrb;
  mrb_funcall(mrb, mrb_app, "setup", 0, NULL);

  if(mrb->exc) {
    mrb_print_error(mrb);
  }

  return message_loop();
}

