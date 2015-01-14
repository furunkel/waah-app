#include <native_app_glue/android_native_app_glue.h>
#include <android/log.h>

#include <jni.h>
#define LOG_TAG "waah"
#undef LOGI
#undef LOGW
#undef LOGE
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

struct user_data {
  mrb_state *mrb;
  app_t *app;
};

struct event {
  int key_code;
  int meta_state;
  int action;
};

typedef struct android_app_s {
  app_t base;
  struct android_app *aapp;
  unsigned int in_main_loop : 1;
  unsigned int pending_show_keyboard : 1;
  unsigned int show_keyboard : 1;
  unsigned int have_focus : 1;
  unsigned int have_event : 1;
  struct event event;
} android_app_t;

#define WAAH_APP_STRUCT android_app_t
static android_app_t *app;


static mrb_value
keyboard_show(mrb_state *mrb, mrb_value self) {
  keyboard_t *keyboard;
  mrb_bool show = TRUE;

  Data_Get_Struct(mrb, self, &_keyboard_type_info, keyboard);
  mrb_get_args(mrb, "|b", &show);

  android_app_t *android_app = (android_app_t *) keyboard->app;

  android_app->pending_show_keyboard = TRUE;
  android_app->show_keyboard = show;

  return self;
}

static mrb_value
keyboard_hide(mrb_state *mrb, mrb_value self) {
  keyboard_t *keyboard;
  Data_Get_Struct(mrb, self, &_keyboard_type_info, keyboard);

  android_app_t *android_app = (android_app_t *) keyboard->app;
  android_app->pending_show_keyboard = TRUE;
  android_app->show_keyboard = FALSE;

  return self;
}

static mrb_value
keyboard_toggle(mrb_state *mrb, mrb_value self) {
  keyboard_t *keyboard;
  Data_Get_Struct(mrb, self, &_keyboard_type_info, keyboard);

  android_app_t *android_app = (android_app_t *) keyboard->app;
  android_app->pending_show_keyboard = TRUE;
  android_app->show_keyboard = !android_app->show_keyboard;

  return self;
}

static mrb_value
app_log(mrb_state *mrb, mrb_value self) {

  mrb_sym level;
  char *msg;
  char *tag = NULL;
  mrb_get_args(mrb, "nz|z", &level, &msg, &tag);

  if(tag == NULL) {
    tag = "waah";
  }

  if(level == mrb_intern_lit(mrb, "warn")) {
    __android_log_print(ANDROID_LOG_WARN, tag, "%s", msg);
  } else if(level == mrb_intern_lit(mrb, "error")) {
    __android_log_print(ANDROID_LOG_ERROR, tag, "%s", msg);
  } else if(level == mrb_intern_lit(mrb, "verbose")) {
    __android_log_print(ANDROID_LOG_VERBOSE, tag, "%s", msg);
  } else if(level == mrb_intern_lit(mrb, "debug")) {
    __android_log_print(ANDROID_LOG_DEBUG, tag, "%s", msg);
  } else if(level == mrb_intern_lit(mrb, "info")) {
    __android_log_print(ANDROID_LOG_INFO, tag, "%s", msg);
  } else if(level == mrb_intern_lit(mrb, "fatal")) {
    __android_log_print(ANDROID_LOG_FATAL, tag, "%s", msg);
  } else {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid log level");
  }
  return self;
}

static void
cat_key_event(mrb_state *mrb, android_app_t *app, int action, int code, int meta_state) {
  jint res;
  JNIEnv *env;
  JavaVM *vm = app->aapp->activity->vm;
  keyboard_t *kb = ((app_t *)app)->keyboard;

  if(!mrb_nil_p(kb->text_blk)) {
    res = (*vm)->AttachCurrentThread(vm, &env, NULL);
    if (res == JNI_ERR) {
        return;
    }

    jclass activityClass = (*env)->FindClass(env, "android/app/NativeActivity");
    jmethodID getClassLoader = (*env)->GetMethodID(env, activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject clazz = (*env)->CallObjectMethod(env, app->aapp->activity->clazz, getClassLoader);
    jclass classLoader = (*env)->FindClass(env, "java/lang/ClassLoader");
    jmethodID findClass = (*env)->GetMethodID(env, classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    jstring strClassName = (*env)->NewStringUTF(env, "org/waah/Keyboard");
    jclass keyboard = (jclass)(*env)->CallObjectMethod(env, clazz, findClass, strClassName); 
    LOGI("Keyboard class (%p)", keyboard);
    jmethodID keyboard_method = (*env)->GetStaticMethodID(env, keyboard, "getBuffer", "()[B");
    LOGI("Getting buffer...(%p)", keyboard_method);
    jbyteArray buffer = (jbyteArray) (*env)->CallStaticObjectMethod(env, keyboard, keyboard_method, action, code, meta_state);

    jboolean is_copy;
    jint len;

    LOGI("Got buffer: %p", buffer);
    if(buffer != NULL) {
      len = (*env)->GetArrayLength(env, buffer);
      LOGI("Got buffer len: %d", len);
      if(len > 0) {
        jbyte *cbuffer = (*env)->GetByteArrayElements(env, buffer, &is_copy);
        LOGI("Got cbuffer: %s", cbuffer);
        mrb_value str = mrb_str_new(mrb, cbuffer, len);
        mrb_funcall(mrb, kb->text_blk, "call", 1, str);
        (*env)->ReleaseByteArrayElements(env, buffer, cbuffer, JNI_ABORT);
      }
    }
    (*vm)->DetachCurrentThread(vm);
  }
  LOGI("Done...");
}


static int
show_keyboard(android_app_t *app, int show) {
  jint res;
  JNIEnv *env;
  JNIEnv *jni;
  jmethodID keyboard_method;
  jboolean success;
  JavaVM *vm = app->aapp->activity->vm;

  res = (*vm)->AttachCurrentThread(vm, &env, NULL);
  if (res == JNI_ERR) {
      return 0;
  }

  jclass activityClass = (*env)->FindClass(env, "android/app/NativeActivity");
  jmethodID getClassLoader = (*env)->GetMethodID(env, activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
  jobject clazz = (*env)->CallObjectMethod(env, app->aapp->activity->clazz, getClassLoader);
  jclass classLoader = (*env)->FindClass(env, "java/lang/ClassLoader");
  jmethodID findClass = (*env)->GetMethodID(env, classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
  jstring strClassName = (*env)->NewStringUTF(env, "org/waah/Keyboard");
  jclass keyboard = (jclass)(*env)->CallObjectMethod(env, clazz, findClass, strClassName); 
  LOGI("Keyboard class: %p", keyboard);

  LOGI("Show keyboard: %d", show);
  keyboard_method = (*env)->GetStaticMethodID(env, keyboard, "setVisible", "(Z)Z");
  success = (*env)->CallStaticBooleanMethod(env, keyboard, keyboard_method, show ? JNI_TRUE : JNI_FALSE);

  (*vm)->DetachCurrentThread(vm);

  return success == JNI_TRUE;
}




static void handle_app_command(struct android_app* aapp, int32_t cmd) {

  struct user_data *user_data = (struct user_data *) aapp->userData;
  android_app_t *android_app = (android_app_t *)user_data->app;

  switch (cmd) {
    case APP_CMD_INIT_WINDOW:
      android_app->have_focus = TRUE;
      break;
    case APP_CMD_LOST_FOCUS:
      android_app->have_focus = FALSE;
      break;
    case APP_CMD_GAINED_FOCUS:
      android_app->have_focus = TRUE;
      break;
  }
}

static int32_t handle_input(struct android_app* aapp, AInputEvent* event) {
  /* app->userData is available here */
  struct user_data *user_data = (struct user_data *) aapp->userData;

  //waah_canvas_t *canvas = (waah_canvas_t *)app->userData;
  app_t *app = (app_t *)user_data->app;
  android_app_t *android_app = (android_app_t *)user_data->app;
  mrb_state *mrb = (mrb_state *)user_data->mrb;

  app->redraw = TRUE;

  if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
    android_app->have_focus = FALSE;
    size_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    if(action == AMOTION_EVENT_ACTION_POINTER_UP ||
       action == AMOTION_EVENT_ACTION_UP ||
       action == AMOTION_EVENT_ACTION_CANCEL) {
      app->pointers[0]->x = -1;
      app->pointers[0]->y = -1;
      app->pointers[0]->released[1] = app->time;
    } else {
      app->pointers[0]->x = AMotionEvent_getX(event, 0);
      app->pointers[0]->y = AMotionEvent_getY(event, 0);
      app->pointers[0]->pressed[1] = app->time;
    }
    return 1;
  } else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {

    int action = AKeyEvent_getAction(event);
    int key_code = AKeyEvent_getKeyCode(event);
    if(action == AKEY_EVENT_ACTION_MULTIPLE|| action == AKEY_EVENT_ACTION_UP) {
      android_app->have_event = 1;
      android_app->event.action = action;
      android_app->event.key_code = key_code;
      android_app->event.meta_state = AKeyEvent_getMetaState(event);
    }

    if(key_code > 0 && key_code < N_KEYS) {
      if(action == AKEY_EVENT_ACTION_DOWN) {
        app->keyboard->pressed[key_code] = app->time;
      } else if(action == AKEY_EVENT_ACTION_UP) {
        app->keyboard->released[key_code] = app->time;
      }
    }

    LOGI("Key event (saved): action=%d keyCode=%d metaState=0x%x",
    AKeyEvent_getAction(event),
    AKeyEvent_getKeyCode(event),
    AKeyEvent_getMetaState(event));
  }
  return 0;
}

static void
_app_run_android(mrb_state *mrb, mrb_value mrb_app, android_app_t *android_app) {

  app_t *app = (app_t *) android_app;
  waah_canvas_t *canvas = (waah_canvas_t *) app;

  LOGI("run_android");
  display_t *display = &app->display;
  keyboard_t *keyboard = app->keyboard;

  LOGI("run_android 2");
  
  struct user_data user_data = {.app = app, .mrb = mrb};

  android_app->aapp->userData = &user_data;
  android_app->aapp->onAppCmd = handle_app_command;
  android_app->aapp->onInputEvent = handle_input;

  int ident;
  int events;
  struct android_poll_source* source;


  LOGI("Calling #setup (%p %p)", mrb, mrb_app);
  mrb_value setup_ret = mrb_funcall(mrb, mrb_app, "setup", 0, NULL);
  LOGI("Called #setup (%f)", mrb_float(setup_ret));

  LOGI("run_android 22 %p", app);
  LOGI("run_android 3 %f", app->rate);
  double secs = (1.0 / (app->rate));
  LOGI("run_android 4");
  app->redraw = TRUE;

  android_app->in_main_loop = TRUE;
  app->time = 0;
  LOGI("Entering main loop");
  while(!app->quit) {
    if(android_app->have_event) {
      android_app->have_event = 0;
      LOGI("Key event (recovered): action=%d keyCode=%d metaState=0x%x",
      android_app->event.action,
      android_app->event.key_code,
      android_app->event.meta_state);

      cat_key_event(mrb, android_app, android_app->event.action, android_app->event.key_code, android_app->event.meta_state);
    }

    while ((ident=ALooper_pollAll(app->redraw ? 0 : -1, NULL, &events, (void**)&source)) >= 0) {
      // Process this event.
      if (source != NULL) {
        source->process(android_app->aapp, source);
      }
      // Check if we are exiting.
      if (android_app->aapp->destroyRequested != 0) {
        LOGI("Engine thread destroy requested!");

        goto done;
      }
    }

    /* Now that we've delt with input, draw stuff */
    if (android_app->aapp->window != NULL) {
      if(app->redraw) {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        unsigned long long ts = tv.tv_sec * 1000000 + tv.tv_usec;
        if((double)(ts - app->last_redraw) / 1000000 > secs) {

          ANativeWindow_Buffer buffer;
          if (ANativeWindow_lock(android_app->aapp->window, &buffer, NULL) < 0) {
            LOGW("Unable to lock window buffer");
            continue;
          }

          display->width = buffer.width;
          display->height = buffer.height;

          if(display->surface != NULL) {
            cairo_surface_destroy(display->surface);
          }

          //if(display->surface == NULL) {
          int pixel_size;
          if (buffer.format == WINDOW_FORMAT_RGBA_8888 || buffer.format == WINDOW_FORMAT_RGBX_8888) {
            pixel_size = 4;
            display->surface = cairo_image_surface_create_for_data(buffer.bits, CAIRO_FORMAT_RGB24, buffer.width, buffer.height, buffer.stride * pixel_size);
            //display->surface2 = cairo_image_surface_create(CAIRO_FORMAT_RGB24, buffer.width, buffer.height);
          } else {
            LOGE("Unsupported buffer format: %d", buffer.format);
            return;
          }
            //cr = cairo_create(display->surface);

          //LOGI("Rendering frame %d", tick);

            app->redraw = FALSE;
            app->last_redraw = ts;

            canvas->cr = cairo_create(display->surface);
            mrb_funcall(mrb, mrb_app, "draw", 0, NULL);
            cairo_destroy(canvas->cr);

            ++app->time;

            int i;
            unsigned char *bits = (unsigned char *) buffer.bits;
            for(i = 0; i < buffer.width * buffer.height * pixel_size; i += pixel_size) {
              unsigned char r,g,b;
              r = bits[i];
              b = bits[i + 2];
              bits[i] = b;
              bits[i + 2] = r;
              bits[i + 3] = 255 / 10;
            }


          ANativeWindow_unlockAndPost(android_app->aapp->window);

          if(android_app->pending_show_keyboard) {
            android_app->pending_show_keyboard = !show_keyboard(android_app, android_app->show_keyboard);
          }

        }
      }
          //LOGI("Calling #draw");
          //display->cr = cairo_create(display->surface);
          //mrb_funcall(mrb, mrb_canvas, "draw", 0, NULL);
          //cairo_destroy(display->cr);

        //}
      //}
    }
  }

done:
  android_app->in_main_loop = FALSE;
  if(display->surface != NULL) {
    cairo_surface_destroy(display->surface);
  }

  LOGI("Leaving main loop");
}



unsigned char *
android_asset_read(const char *filename, long *len) {

  AAssetManager* mgr = app->aapp->activity->assetManager;
  AAsset* asset = AAssetManager_open(mgr, filename, AASSET_MODE_UNKNOWN);
  if(asset == NULL){
    LOGE("asset %s not found", filename);
    return;
  };

  long size = AAsset_getLength(asset);
  unsigned char* buffer = (char*) malloc (sizeof(unsigned char)*(size + 1));
  AAsset_read(asset, buffer, size);
  AAsset_close(asset);
  buffer[size] = '\0';

  if(len != NULL) *len = size;

  return buffer;
}

FILE *
android_raw_asset_fopen(const char *filename) {
  AAssetManager* mgr = app->aapp->activity->assetManager;
  if (NULL == mgr) return NULL;

  AAsset* asset = AAssetManager_open(mgr, filename, AASSET_MODE_UNKNOWN);
  if(asset == NULL) {
      return NULL;
  }
  off_t start, length;
  int fd = AAsset_openFileDescriptor(asset, &start, &length);
  if (fd < 0)
    return NULL;

  FILE *file = fdopen(fd, "r");
  fseek(file, start, SEEK_SET);

  return file;
}

static int 
load_png_from_asset(mrb_state *mrb, waah_image_t *image, const char *filename) {
  long len;
  unsigned char *buffer = android_asset_read(filename, &len);

  return _waah_load_png_from_buffer(mrb, image, buffer, len);
}

static int
load_jpeg_from_asset(mrb_state *mrb, waah_image_t *image, const char *filename) {
  LOGI("opening file: %s", filename);
  FILE *file = android_raw_asset_fopen(filename);
  LOGI("opened file: %p", file);

  return _waah_load_jpeg_from_file(mrb, image, file);
}


static mrb_value
image_load_asset(mrb_state *mrb, mrb_value self) {
  return _waah_image_load(mrb, self, load_png_from_asset, load_jpeg_from_asset);
}




#include <native_app_glue/android_native_app_glue.c>
