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

typedef enum {
  WAAH_LOG_LEVEL_VERBOSE,
  WAAH_LOG_LEVEL_DEBUG,
  WAAH_LOG_LEVEL_INFO,
  WAAH_LOG_LEVEL_WARN,
  WAAH_LOG_LEVEL_ERROR,
  WAAH_LOG_LEVEL_FATAL,
  WAAH_LOG_LEVEL_UNKNOWN,
  WAAH_N_LOG_LEVELS
} waah_log_level_t;

waah_log_level_t _waah_log_level = WAAH_LOG_LEVEL_WARN;

static const char *const log_levels[WAAH_N_LOG_LEVELS] = {
  "VERBOSE",
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
  "FATAL",
  "UNKNOWN"
};

static const char *const log_levels_lc[WAAH_N_LOG_LEVELS] = {
  "verbose",
  "debug",
  "info",
  "warn",
  "error",
  "fatal",
  "unknown"
};


#define waah_info(format, ...) waah_log(WAAH_LOG_LEVEL_INFO, _waah_log_tag, format, ##__VA_ARGS__)
#define waah_warn(format, ...) waah_log(WAAH_LOG_LEVEL_WARN, _waah_log_tag, format, ##__VA_ARGS__)
#define waah_error(format, ...) waah_log(WAAH_LOG_LEVEL_ERROR, _waah_log_tag, format, ##__VA_ARGS__)
#define waah_fatal(format, ...) waah_log(WAAH_LOG_LEVEL_FATAL, _waah_log_tag, format, ##__VA_ARGS__)

void
waah_log(waah_log_level_t level, const char *tag, const char *format, ...)
{
  if(level < _waah_log_level) return;

  va_list args;

  const char *prefix = "waah:";
  const char *sep1 = ":";
  const char *sep2 = ": ";

  size_t prefix_len = strlen(prefix);
  size_t level_len = strlen(log_levels[level]);
  size_t tag_len = strlen(tag);
  size_t sep1_len = strlen(sep1);
  size_t sep2_len = strlen(sep2);
  size_t format_len = strlen(format);

  char full_format[prefix_len
                   + level_len
                   + sep1_len
                   + tag_len
                   + sep2_len
                   + format_len
                   + 2];

  size_t i = 0;
  memcpy(full_format + i, prefix, prefix_len); i += prefix_len;
  memcpy(full_format + i, log_levels[level], level_len); i += level_len;
  memcpy(full_format + i, sep1, sep1_len); i += sep1_len;
  memcpy(full_format + i, tag, tag_len); i += tag_len;
  memcpy(full_format + i, sep2, sep2_len); i += sep2_len;
  memcpy(full_format + i, format, format_len); i += format_len;
  full_format[i] = '\n'; i++;
  full_format[i] = '\0'; i++;

  va_start(args, format);
  vfprintf(stderr, full_format, args);
  va_end(args);
}


static mrb_value
app_log_stderr(mrb_state *mrb, mrb_value self) {
  mrb_sym level;
  char *msg;
  char *tag = NULL;
  mrb_get_args(mrb, "nz|z", &level, &msg, &tag);

  if(tag == NULL) {
    tag = ((app_t *)app)->title;
  }

  if(level == mrb_intern_lit(mrb, "verbose")) {
    waah_log(WAAH_LOG_LEVEL_VERBOSE, tag, "%s", msg);
  } else if(level == mrb_intern_lit(mrb, "debug")) {
    waah_log(WAAH_LOG_LEVEL_DEBUG, tag, "%s", msg);
  } else if(level == mrb_intern_lit(mrb, "info")) {
    waah_log(WAAH_LOG_LEVEL_INFO, tag, "%s", msg);
  } else if(level == mrb_intern_lit(mrb, "warn")) {
    waah_log(WAAH_LOG_LEVEL_WARN, tag, "%s", msg);
  } else if(level == mrb_intern_lit(mrb, "error")) {
    waah_log(WAAH_LOG_LEVEL_ERROR, tag, "%s", msg);
  } else if(level == mrb_intern_lit(mrb, "fatal")) {
    waah_log(WAAH_LOG_LEVEL_FATAL, tag, "%s", msg);
  } else {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid log level");
  }

  return self;
}

static mrb_value
app_log_level(mrb_state *mrb, mrb_value self) {
  mrb_sym level;

  if(mrb_get_args(mrb, "|n", &level) > 0) {
    if(level == mrb_intern_lit(mrb, "verbose")) {
      _waah_log_level = WAAH_LOG_LEVEL_VERBOSE;
    } else if(level == mrb_intern_lit(mrb, "debug")) {
      _waah_log_level = WAAH_LOG_LEVEL_DEBUG;
    } else if(level == mrb_intern_lit(mrb, "info")) {
      _waah_log_level = WAAH_LOG_LEVEL_INFO;
    } else if(level == mrb_intern_lit(mrb, "warn")) {
      _waah_log_level = WAAH_LOG_LEVEL_WARN;
    } else if(level == mrb_intern_lit(mrb, "error")) {
      _waah_log_level = WAAH_LOG_LEVEL_ERROR;
    } else if(level == mrb_intern_lit(mrb, "fatal")) {
      _waah_log_level = WAAH_LOG_LEVEL_FATAL;
    } else {
      mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid log level");
    }
  } 

  return mrb_symbol_value(mrb_intern_cstr(mrb, log_levels_lc[_waah_log_level]));
}



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
return_nil_m(mrb_state *mrb, mrb_value self) {
  return mrb_nil_value();
}

static mrb_value
return_true_m(mrb_state *mrb, mrb_value self) {
  return mrb_true_value();
}

static mrb_value
return_self_m(mrb_state *mrb, mrb_value self) {
  return self;
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


  mrb_define_method(mrb, cKeyboard, "show", return_self_m, ARGS_OPT(1));
  mrb_define_method(mrb, cKeyboard, "toggle", return_self_m, ARGS_NONE());
  mrb_alias_method(mrb, cKeyboard, mrb_intern_cstr(mrb, "visible"), mrb_intern_cstr(mrb, "show"));
  mrb_define_method(mrb, cKeyboard, "hide", return_self_m, ARGS_NONE());


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
  LOGI("Getting file name...");
  char *file_name = get_file_name(aapp);
  LOGI("Looking for file %s %p", file_name, mgr);
  AAsset* asset = AAssetManager_open(mgr, file_name, AASSET_MODE_UNKNOWN);
  free(file_name);

  if(asset == NULL){
    LOGE("%s not found in assets", file_name);
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
  mrb_define_method(mrb, cApp, "log", app_log_android, ARGS_REQ(2) | ARGS_OPT(1));
  mrb_define_method(mrb, cApp, "log_level", return_nil_m, ARGS_OPT(1));
#else
  mrb_define_method(mrb, cApp, "log", app_log_stderr, ARGS_REQ(2) | ARGS_OPT(1));
  mrb_define_method(mrb, cApp, "android?", return_false_m, ARGS_NONE());
  mrb_define_method(mrb, cApp, "log_level", app_log_level, ARGS_OPT(1));

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

