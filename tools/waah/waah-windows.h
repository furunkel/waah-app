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
  wchar_t last_char;
  unsigned int surrogate_char : 1;
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


void init_window_classex(PWNDCLASSEX class,
                         window_instance *win,
                         HINSTANCE hInstance,
                         WNDPROC WndProc,
                         LPCSTR name)
{
  WNDCLASSEX mine = {    // What a fucking pain in the ass.
    // never changing:
    .cbSize = sizeof(WNDCLASSEX),
    .style = 0,
    .cbClsExtra = 0,
    .cbWndExtra = 0,

    // supplied by user:
    .lpfnWndProc = WndProc,
    .hInstance = hInstance,
    .lpszClassName = name,

    // sensible defaults:
    .hIcon = LoadIcon(NULL, IDI_APPLICATION),
    .hCursor = LoadCursor(NULL, IDC_ARROW),
    .hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH),
    .lpszMenuName = NULL,
    .hIconSm = LoadIcon(NULL, IDI_WINLOGO),
  };

  *class = mine;
  win->class = class;
  win->hWnd = NULL;
  win->title = "Anonymous coward window";  // default value
  win->style = WS_OVERLAPPEDWINDOW;
  win->x = win->y = CW_USEDEFAULT;
}

int create_window(window_instance *win, int width, int height)
{
  win->hWnd = CreateWindow(win->class->lpszClassName,
                           win->title,
                           win->style,
                           win->x, win->y,
                           width, height,
                           NULL,    // parent handle
                           NULL,    // menu handle
                           win->class->hInstance,
                           NULL);
  return !!win->hWnd;
}

int message_loop()
{
  MSG msg;

  // params: message, hWnd, first, last
  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}

int _check(char *contents, int result)
{
  char errcodespace[32];
  if (result) return result;
  MessageBox(NULL, contents, "fatal _check error: this code failed", MB_OK);
  sprintf(errcodespace, "error code %d", (int)GetLastError());
  MessageBox(NULL, errcodespace, "here's why", MB_OK);
  ExitProcess(1);
}


// for things that you want to MessageBox about and exit if they fail
#define check(foo) pcheck(foo, int)
#define pcheck(foo, type) (type)_check(#foo, (int)foo)
int _check(char *contents, int result);


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
  waah_canvas_t *canvas = (waah_canvas_t *) app;
  app_t *base_app = (app_t *) app;
  pointer_t *mouse = base_app->pointers[0];
  keyboard_t *keyboard = base_app->keyboard;

  switch(msg) {
  case WM_MOUSEMOVE: 
    {
      HDC hDC = GetDC(hWnd);
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      mouse->prev_x = mouse->x;
      mouse->prev_y = mouse->y;
      mouse->x = x;
      mouse->y = y;
      ReleaseDC(hWnd, hDC);
      break;
    }
  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
    mouse->pressed[msg == WM_LBUTTONDOWN ? 1 : 2] = base_app->time;
    break;
  case WM_LBUTTONUP:
  case WM_RBUTTONUP:
    mouse->released[msg == WM_LBUTTONUP ? 1 : 2] = base_app->time;
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  case WM_TIMER:
    RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
    break;
  case WM_EXITSIZEMOVE:
    break;
  case WM_CHAR: {
    wchar_t str[2];
    int wlen;

    // handles surrogates
    // see http://unicodebook.readthedocs.org/en/latest/unicode_encodings.html#utf-16-surrogate-pairs for the ranges

    // low surrogate
    if(wParam >= 0xDC00 && wParam <= 0xDFFF) {
      str[0] = app->last_char;
      str[1] = wParam;
      wlen = 2;
    } else {
      str[0] = wParam;
      wlen = 1;
    }
    app->last_char = wParam;

    // if not a high surrogate
    if(!(wParam >= 0xD800 && wParam <= 0xDBFF)) {
      int len = WideCharToMultiByte(CP_UTF8, 0, str, wlen, NULL, 0, NULL, NULL);
      char *utf8 = malloc(sizeof(char) * len);
      WideCharToMultiByte(CP_UTF8, 0, str, wlen, utf8, len, NULL, NULL);

      if(!mrb_nil_p(keyboard->text_blk)) {
        mrb_value mrb_str = mrb_str_new(app->mrb, utf8, len);
        mrb_funcall(app->mrb, keyboard->text_blk, "call", 1, mrb_str);
      }
      free(utf8);
    }
    break;
  }
  case WM_KEYDOWN:
    if(wParam < N_KEYS) {
      keyboard->pressed[wParam] = base_app->time;
    }
    break;
  case WM_KEYUP:
    if(wParam < N_KEYS) {
      keyboard->released[wParam] = base_app->time;
    }
    break;
  case WM_PAINT: 
    {
      PAINTSTRUCT paintStruct;
      HDC hDC = BeginPaint(hWnd, &paintStruct);
      canvas->surface = cairo_win32_surface_create(hDC);
      canvas->cr = cairo_create(canvas->surface);
      mrb_funcall(app->mrb, mrb_app, "draw", 0, NULL);

      cairo_surface_finish(canvas->surface);
      cairo_destroy(canvas->cr);
      cairo_surface_destroy(canvas->surface);
      EndPaint(hWnd, &paintStruct);
      base_app->time++;
      break;
    }
  default:
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }
  return 0;
}

static void
_app_run_windows(mrb_state *mrb, mrb_value mrb_app, windows_app_t *windows_app) {
  waah_canvas_t *canvas = (waah_canvas_t *) windows_app;
  app_t *app = (app_t *) windows_app;
  WNDCLASSEX windowClass;
  window_instance win;
  init_window_classex(&windowClass, &win, windows_app->hInstance, WndProc, "AClass");
  if (!RegisterClassEx(&windowClass)) return;
  win.title = app->title;
  if (!create_window(&win, canvas->width, canvas->height)) return;

  windows_app->mrb = mrb;
  app->time = 0;

  ShowWindow(win.hWnd, windows_app->nCmdShow);

  mrb_funcall(mrb, mrb_app, "setup", 0, NULL);

  if(mrb->exc) {
    mrb_print_error(mrb);
  }

  UINT el = (1.0 / app->rate) * 1000;
  SetTimer(win.hWnd,   // handle to main window 
    1,                 // timer identifier 
    el,                // 10-second interval 
    (TIMERPROC) NULL); // no timer callback 

  return message_loop();
}

