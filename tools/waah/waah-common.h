struct app_s;

#define MAX(a,b) (((a)>(b))?(a):(b))

#define N_KEYS 255
typedef struct keyboard_s {
  unsigned long pressed[N_KEYS];
  unsigned long released[N_KEYS];
  mrb_value text_blk;
  struct app_s *app;
} keyboard_t;

#define N_BUTTONS 5
typedef struct pointer_s {
  int id;
  int x;
  int y;
  int prev_x;
  int prev_y;
  unsigned long pressed[N_BUTTONS];
  unsigned long released[N_BUTTONS];
  long sleep;
  struct app_s *app;
} pointer_t;

typedef struct display_s {
  cairo_surface_t *surface;
  cairo_surface_t *surface2;
  int width;
  int height;
} display_t;

#define N_POINTERS 10
typedef struct app_s {
  waah_canvas_t base;
  unsigned long time;
  keyboard_t *keyboard;
  mrb_value mrb_keyboard;
  pointer_t *pointers[N_POINTERS];
  mrb_value mrb_pointers[N_POINTERS];
  display_t display;
  unsigned int quit : 1;
  unsigned int redraw : 1;
  unsigned long long last_redraw;
  double rate;
  char *title;
} app_t;

struct RClass *cKeyboard;
struct RClass *cPointer;
struct RClass *cApp;

static void
pointer_free(mrb_state *mrb, void *ptr) {
  mrb_free(mrb, ptr);
}

static void
keyboard_free(mrb_state *mrb, void *ptr) {
  mrb_free(mrb, ptr);
}

static void
app_free(mrb_state *mrb, void *ptr) {
  app_t *app = (app_t *) ptr;
  if(app->title != NULL) {
    mrb_free(mrb, app->title);
  }
}

static struct mrb_data_type _pointer_type_info = {"Pointer", pointer_free};
static struct mrb_data_type _keyboard_type_info = {"Keyboard", keyboard_free};
//static struct mrb_data_type _waah_canvas_type_info = {"App", app_free}
extern struct mrb_data_type _waah_canvas_type_info;

static void
_path_contains(cairo_t *cr, int x1, int y1, int x2, int y2, int *r1, int *r2) {
  *r1 = cairo_in_fill(cr, (double) x1, (double) y1);
  if(r2 != NULL) {
    *r2 = cairo_in_fill(cr, (double) x2, (double) y2);
  }
}

static void
_path_extents_contains(cairo_t *cr, int x1, int y1, int x2, int y2, int *r1, int *r2) {
  double ex1, ex2, ey1, ey2;
  cairo_path_extents(cr, &ex1, &ey1, &ex2, &ey2);

  *r1 = x1 >= ex1 && x1 <= ex2 &&
        y1 >= ey1 && y1 <= ey2;

  if(r2 != NULL) {
    *r2 = x2 >= ex1 && x2 <= ex2 &&
          y2 >= ey1 && y2 <= ey2;
  }
}

static mrb_value
pointer_down(mrb_state *mrb, mrb_value self) {

  pointer_t *pointer;
  mrb_int button = 1;

  Data_Get_Struct(mrb, self, &_pointer_type_info, pointer);
  mrb_get_args(mrb, "|i", &button);

  if(pointer->sleep > 0) return mrb_false_value();

  if(button <= 0 || button > N_BUTTONS) {
    return mrb_false_value();
  }

  if(pointer->pressed[button] <= pointer->released[button]) {
    return mrb_false_value();
  } else {
    return mrb_true_value();
  }
}


static mrb_value
pointer_pressed(mrb_state *mrb, mrb_value self) {

  pointer_t *pointer;
  mrb_int button = 1;

  Data_Get_Struct(mrb, self, &_pointer_type_info, pointer);
  mrb_get_args(mrb, "|i", &button);

  if(button <= 0 || button > N_BUTTONS) {
    return mrb_false_value();
  }

  if(pointer->pressed[button] != pointer->app->time) {
    return mrb_false_value();
  } else {
    return mrb_true_value();
  }
}

static mrb_value
pointer_in(mrb_state *mrb, mrb_value self) {
  pointer_t *pointer;
  int r;
  double x, y, w, h;
  int argc;

  Data_Get_Struct(mrb, self, &_pointer_type_info, pointer);
  argc = mrb_get_args(mrb, "|ffff", &x, &y, &w, &h);

  if(argc == 4) {
    double x1, y1;
    double ex1, ex2, ey1, ey2;

    x1 = pointer->x;
    y1 = pointer->y;

    ex1 = x;
    ey1 = y;
    ex2 = x + w;
    ey2 = y + h;

    r = x1 >= ex1 && x1 <= ex2 &&
        y1 >= ey1 && y1 <= ey2;
  } else if(argc == 0) {
    _path_contains(((waah_canvas_t *)pointer->app)->cr, pointer->x, pointer->y, 0, 0, &r, NULL);
  } else {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid number of arguments (pass 4 or 0)");
  }

  return r ? mrb_true_value() : mrb_false_value();
}

static mrb_value
pointer_in_extents(mrb_state *mrb, mrb_value self) {
  pointer_t *pointer;
  int r;

  Data_Get_Struct(mrb, self, &_pointer_type_info, pointer);
  _path_extents_contains(((waah_canvas_t *)pointer->app)->cr, pointer->x, pointer->y, 0, 0, &r, NULL);

  return r ? mrb_true_value() : mrb_false_value();
}

static mrb_value
pointer_x(mrb_state *mrb, mrb_value self) {
  pointer_t *pointer;
  Data_Get_Struct(mrb, self, &_pointer_type_info, pointer);
  return mrb_fixnum_value(pointer->x);
}

static mrb_value
pointer_y(mrb_state *mrb, mrb_value self) {
  pointer_t *pointer;
  Data_Get_Struct(mrb, self, &_pointer_type_info, pointer);
  return mrb_fixnum_value(pointer->y);
}

static mrb_value
keyboard_text(mrb_state *mrb, mrb_value self) {
  keyboard_t *keyboard;
  mrb_value blk;
  Data_Get_Struct(mrb, self, &_keyboard_type_info, keyboard);

  mrb_get_args(mrb, "&", &blk);

  return keyboard->text_blk = blk;

  return self;
}

static mrb_value
app_keyboard(mrb_state *mrb, mrb_value self) {
  app_t *app;
  Data_Get_Struct(mrb, self, &_waah_canvas_type_info, app);

  return app->mrb_keyboard;
}

static mrb_value
app_pointer(mrb_state *mrb, mrb_value self) {
  app_t *app;
  mrb_int id = 0;
  Data_Get_Struct(mrb, self, &_waah_canvas_type_info, app);

  mrb_get_args(mrb, "|i", &id);

  if(id < 0 || id >= N_POINTERS) {
    return mrb_nil_value();
  }
  return app->mrb_pointers[id];
}

static mrb_value
keyboard_down(mrb_state *mrb, mrb_value self) {
  keyboard_t *keyboard;
  mrb_int key = 1;

  Data_Get_Struct(mrb, self, &_keyboard_type_info, keyboard);
  mrb_get_args(mrb, "i", &key);

  if(key < 0 || key > N_KEYS) {
    return mrb_false_value();
  }

  if(keyboard->pressed[key] < keyboard->released[key]) {
    return mrb_false_value();
  } else {
    return mrb_true_value();
  }
}


static mrb_value
keyboard_pressed(mrb_state *mrb, mrb_value self) {

  keyboard_t *keyboard;
  mrb_int key = 1;

  Data_Get_Struct(mrb, self, &_keyboard_type_info, keyboard);
  mrb_get_args(mrb, "i", &key);

  if(key < 0 || key > N_KEYS) {
    return mrb_false_value();
  }

  if(keyboard->pressed[key] != keyboard->app->time) {
    return mrb_false_value();
  } else {
    return mrb_true_value();
  }
}

static void
pointers_unsleep(struct app_s *app, long msecs) {
  int i;
  for(i = 0; i < N_POINTERS; i++) {
    app->pointers[i]->sleep = MAX(app->pointers[i]->sleep - msecs, 0);
    fprintf(stderr, "%d %ld %ld\n",i, app->pointers[i]->sleep, msecs);
  }
}

static mrb_value
pointer_sleep(mrb_state *mrb, mrb_value self) {

  pointer_t *pointer;
  double secs;
  Data_Get_Struct(mrb, self, &_pointer_type_info, pointer);
  mrb_get_args(mrb, "f", &secs);

  pointer->sleep = secs * 1000;
  return self;
}
