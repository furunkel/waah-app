#include "mruby.h"
#include "mruby/data.h"
#include "mruby/class.h"
#include "mruby/compile.h"
#include "mruby/variable.h"
#include "mruby/string.h"
#include "mruby/range.h"
#include "mruby/array.h"

#include "waah-canvas.h"

#include <stdlib.h>
#include <sys/time.h>

#include "waah-common.h"

static mrb_value mrb_app;

#if defined(WAAH_PLATFORM_ANDROID)
#include "waah-android.h"
#elif defined(WAAH_PLATFORM_X11)
#include "waah-x11.h"
#elif defined(WAAH_PLATFORM_WINDOWS)
#include "waah-windows.h"
#else
#error Unknown platform
#endif


static mrb_value
app_initialize(mrb_state *mrb, mrb_value self) {
  app_t *app = (app_t *) mrb_calloc(mrb, sizeof(WAAH_APP_STRUCT), 1);
  waah_canvas_t *canvas = (waah_canvas_t *) app;
  mrb_int w, h;
  mrb_value mrb_keyboard;
  mrb_value mrb_pointers;
  size_t title_len;
  char *title = NULL;

  DATA_PTR(self) = app;
  DATA_TYPE(self) = &_waah_canvas_type_info;

  mrb_get_args(mrb, "ii|s", &w, &h, &title, &title_len);

  app->redraw = TRUE;
  app->rate = 1.0;

  if(title == NULL) {
    title = "Waah";
    title_len = strlen(title);
  }
  app->title = mrb_calloc(mrb, sizeof(char), title_len + 1);
  memcpy(app->title, title, title_len);

  canvas->width = w;
  canvas->height = h;
  canvas->free_func = app_free;

  {
    keyboard_t *keyboard = (keyboard_t *) mrb_calloc(mrb, sizeof(keyboard_t), 1);
    mrb_keyboard = mrb_class_new_instance(mrb, 0, NULL, cKeyboard);
    keyboard->text_blk = mrb_nil_value();
    app->keyboard = keyboard;
    keyboard->app = app;
    app->mrb_keyboard = mrb_keyboard;
    DATA_PTR(mrb_keyboard) = keyboard;
    DATA_TYPE(mrb_keyboard) = &_keyboard_type_info;
  }

  {
    int i;
    mrb_pointers = mrb_ary_new_capa(mrb, N_POINTERS);
    for(i = 0; i < N_POINTERS; i++) {
      mrb_value mrb_pointer = mrb_class_new_instance(mrb, 0, NULL, cPointer);
      pointer_t *pointer = (pointer_t *) mrb_calloc(mrb, sizeof(pointer_t), 1);
      pointer->id = i;
      app->pointers[i] = pointer;
      pointer->app = app;
      app->mrb_pointers[i] = mrb_pointer;
      DATA_PTR(mrb_pointer) = pointer;
      DATA_TYPE(mrb_pointer) = &_pointer_type_info;
      mrb_ary_push(mrb, mrb_pointers, mrb_pointer);
    }
  }

  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "__pointers__"), mrb_pointers);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "__keyboard__"), mrb_keyboard);

  return self;
}


static mrb_value
app_run(mrb_state *mrb, mrb_value self) {
  Data_Get_Struct(mrb, self, &_waah_canvas_type_info, app);

#ifdef MRB_APP_PLATFORM_ANDROID
  LOGI("#run called");
#endif

  mrb_gv_set(mrb, mrb_intern_lit(mrb, "$__app__"), self);
  mrb_gv_set(mrb, mrb_intern_lit(mrb, "$app"), self);
  mrb_app = self;

  return self;
}

static mrb_value
app_setup(mrb_state *mrb, mrb_value self) {
  return self;
}


static mrb_value
app_redraw(mrb_state *mrb, mrb_value self) {

  app_t *app;
  Data_Get_Struct(mrb, self, &_waah_canvas_type_info, app);

  app->redraw = TRUE;

  return self;
}

static mrb_value
app_rate(mrb_state *mrb, mrb_value self) {
  app_t *app;
  mrb_float rate;

  Data_Get_Struct(mrb, self, &_waah_canvas_type_info, app);

  if(mrb_get_args(mrb, "|f", &rate) == 1) {
    app->rate = (double)rate;
  }

  return mrb_float_value(mrb, (mrb_float)app->rate);
}


static mrb_value
return_false_m(mrb_state *mrb, mrb_value self) {
  return mrb_false_value();
}

static mrb_value
return_true_m(mrb_state *mrb, mrb_value self) {
  return mrb_true_value();
}

#if defined(WAAH_PLATFORM_ANDROID)
void android_main(struct android_app* aapp) {
  mrb_value v;
  LOGE("android_main %p", aapp);

#elif defined(WAAH_PLATFORM_WINDOWS)
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow) {

  int argc;
  char** argv;
  char* filename = NULL;
  FILE *file;
  mrb_value v;

  {
    int i;
    wchar_t* wCmdLine = GetCommandLineW();
    wchar_t** wArgv = CommandLineToArgvW(wCmdLine, &argc);
    argv = calloc(sizeof(char), argc);
    for(i = 0; i < argc; i++) {
      int len = WideCharToMultiByte(CP_UTF8, 0, wArgv[i], -1, NULL, 0, NULL, NULL);
      argv[i] = malloc(sizeof(char) * len);
      WideCharToMultiByte(CP_UTF8, 0, wArgv[i], -1, argv[i], len, NULL, NULL);
   }
    LocalFree(wArgv);
  }
#else
int main(int argc, char **argv) {
  char* filename = NULL;
  mrb_value v;
  FILE *file;
#endif

  mrb_state *mrb = mrb_open();
  struct RClass *mWaah = mrb_module_get(mrb, "Waah");
  struct RClass *cCanvas = mrb_class_get_under(mrb, mWaah, "Canvas");
  cApp = mrb_define_class_under(mrb, mWaah, "App", cCanvas);
  MRB_SET_INSTANCE_TT(cApp, MRB_TT_DATA);
  mrb_define_method(mrb, cApp, "initialize", app_initialize, ARGS_REQ(2) | ARGS_OPT(1));

  cPointer = mrb_define_class_under(mrb, cApp, "Pointer", mrb->object_class);
  MRB_SET_INSTANCE_TT(cPointer, MRB_TT_DATA);

  cKeyboard = mrb_define_class_under(mrb, cApp, "Keyboard", mrb->object_class);
  MRB_SET_INSTANCE_TT(cKeyboard, MRB_TT_DATA);

  mrb_define_method(mrb, cApp, "run", app_run, ARGS_NONE());
  mrb_define_method(mrb, cApp, "redraw", app_redraw, ARGS_NONE());
  mrb_define_method(mrb, cApp, "setup", app_setup, ARGS_NONE());
  mrb_define_method(mrb, cApp, "rate", app_rate, ARGS_OPT(1));
  mrb_define_method(mrb, cApp, "keyboard", app_keyboard, ARGS_NONE());
  mrb_define_method(mrb, cApp, "pointer", app_pointer, ARGS_OPT(1));
  mrb_alias_method(mrb, cApp, mrb_intern_cstr(mrb, "mouse"), mrb_intern_cstr(mrb, "pointer"));

  mrb_define_method(mrb, cPointer, "down?", pointer_down, ARGS_OPT(1));
  mrb_define_method(mrb, cPointer, "pressed?", pointer_pressed, ARGS_OPT(1));
  mrb_define_method(mrb, cPointer, "in?", pointer_in, ARGS_NONE());
  mrb_define_method(mrb, cPointer, "x", pointer_x, ARGS_NONE());
  mrb_define_method(mrb, cPointer, "y", pointer_y, ARGS_NONE());

  mrb_define_method(mrb, cKeyboard, "text", keyboard_text, ARGS_BLOCK());
  mrb_define_method(mrb, cKeyboard, "down?", keyboard_down, ARGS_REQ(1));
  mrb_define_method(mrb, cKeyboard, "pressed?", keyboard_pressed, ARGS_REQ(1));


#ifdef WAAH_PLATFORM_ANDROID

  LOGI("Registered common methods...");

  mrb_define_method(mrb, cKeyboard, "show", keyboard_show, ARGS_OPT(1));

  mrb_define_method(mrb, cKeyboard, "toggle", keyboard_toggle, ARGS_NONE());
  mrb_alias_method(mrb, cKeyboard, mrb_intern_cstr(mrb, "visible"), mrb_intern_cstr(mrb, "show"));
  mrb_define_method(mrb, cKeyboard, "hide", keyboard_hide, ARGS_NONE());

  struct RClass *cImage = mrb_class_get_under(mrb, mWaah, "Image");
  mrb_define_class_method(mrb, cImage, "asset", image_load_asset, ARGS_REQ(1));

  struct RClass *cFont = mrb_class_get_under(mrb, mWaah, "Font");
  mrb_define_class_method(mrb, cFont, "asset", font_load_asset, ARGS_REQ(1));
  LOGI("Registered android specific methods...");

  AAssetManager* mgr = aapp->activity->assetManager;

  LOGI("Looking for app.rb %p", mgr);
  AAsset* asset = AAssetManager_open(mgr, "app.rb", AASSET_MODE_UNKNOWN);
  if(asset == NULL){
    LOGE("app.rb not found in assets");
    return;
  };

  long size = AAsset_getLength(asset);
  char* buffer = (char*) malloc (sizeof(char)*(size + 1));
  AAsset_read(asset, buffer, size);
  AAsset_close(asset);
  buffer[size] = '\0';

  LOGI("Evaluating ´%s'", buffer);
  v = mrb_load_string(mrb, buffer);

  mrb_define_method(mrb, cApp, "android?", return_true_m, ARGS_NONE());
  mrb_define_method(mrb, cApp, "log", app_log, ARGS_REQ(2) | ARGS_OPT(1));

#else

  mrb_define_method(mrb, cApp, "android?", return_false_m, ARGS_NONE());

  if(argc > 1) {
    filename = argv[1];
  } else {
    fprintf(stderr, "No filename given. Set MRUBY_CANVAS_FILENAME\n");
  }
  file = fopen(filename, "r");
  v = mrb_load_file(mrb, file);

#endif

  if(app == NULL) {
    const char *msg = "No app found. Did you call App#run ?";
#ifdef WAAH_PLATFORM_ANDROID
    LOGI(msg);
#endif
    mrb_raise(mrb, E_RUNTIME_ERROR, msg);
  } else {
#ifdef WAAH_PLATFORM_ANDROID
  ((android_app_t *)app)->aapp = aapp;
#endif

#ifdef WAAH_PLATFORM_WINDOWS
  ((windows_app_t *)app)->hInstance = hInstance;
  ((windows_app_t *)app)->nCmdShow = nCmdShow;
#endif

  }

#if defined(WAAH_PLATFORM_X11)
  _app_run_xlib(mrb, mrb_app, app);
#elif defined(WAAH_PLATFORM_ANDROID)
  app_dummy();
  LOGI("Starting android main loop");
  _app_run_android(mrb, mrb_app, app);
  LOGI("Leaving android main loop");
#elif defined(WAAH_PLATFORM_WINDOWS)
  _app_run_windows(mrb, mrb_app, app);
#else
#error No platform
#endif

  if(mrb->exc) {
    mrb_print_error(mrb);
  }

  mrb_close(mrb);


#ifndef WAAH_PLATFORM_ANDROID
  return 0;
#endif
}


#ifdef WAAH_PLATFORM_ANDROID
int main(int argc, char **argv) {
  app_dummy();
  ANativeActivity_onCreate(NULL, NULL, 0);
  return 0;
}
#endif

