/*
 * DuoStats Watchface for Pebble 2 (SDK 4.5)
 *
 * Layout:
 *  +------------------+
 *  |   HH:MM (large)  |  <- top half
 *  +--------+---------+
 *  | Box A  |  Box B  |  <- row 1
 *  +--------+---------+
 *  | Box C  |  Box D  |  <- row 2
 *  +--------+---------+
 *
 * Box data sources: Steps | Distance (m) | Calories | Sleep
 * Settings via phone app: dark/light mode + per-box data source
 */

#include <pebble.h>

// -- Message keys (match package.json messageKeys) --------------------------
extern uint32_t MESSAGE_KEY_DARK_MODE;
extern uint32_t MESSAGE_KEY_BOX_A;
extern uint32_t MESSAGE_KEY_BOX_B;
extern uint32_t MESSAGE_KEY_BOX_C;
extern uint32_t MESSAGE_KEY_BOX_D;

// -- Persistent storage keys ------------------------------------------------
#define PERSIST_DARK_MODE  10
#define PERSIST_BOX_A      11
#define PERSIST_BOX_B      12
#define PERSIST_BOX_C      13
#define PERSIST_BOX_D      14

// -- Data source IDs --------------------------------------------------------
#define DATA_STEPS      0
#define DATA_DISTANCE   1
#define DATA_CALORIES   2
#define DATA_SLEEP      3

// -- Globals ----------------------------------------------------------------
static Window   *s_window;
static Layer    *s_canvas_layer;

static bool  s_dark_mode       = true;
static int   s_box_cfg[4]      = { DATA_STEPS, DATA_DISTANCE, DATA_CALORIES, DATA_SLEEP };

static int32_t  s_steps    = 0;
static int32_t  s_distance = 0;
static int32_t  s_calories = 0;
static int32_t  s_sleep    = 0;   // minutes

static int s_hours   = 0;
static int s_minutes = 0;

static GFont s_font_time;
static GFont s_font_label;
static GFont s_font_value;

// -- Helpers ----------------------------------------------------------------

static const char *data_label(int id) {
  switch (id) {
    case DATA_STEPS:    return "STEPS";
    case DATA_DISTANCE: return "DIST m";
    case DATA_CALORIES: return "CALS";
    case DATA_SLEEP:    return "SLEEP";
    default:            return "---";
  }
}

static void data_value_str(int id, char *buf, size_t len) {
  switch (id) {
    case DATA_STEPS:
      snprintf(buf, len, "%ld", (long)s_steps);
      break;
    case DATA_DISTANCE:
      snprintf(buf, len, "%ld", (long)s_distance);
      break;
    case DATA_CALORIES:
      snprintf(buf, len, "%ld", (long)s_calories);
      break;
    case DATA_SLEEP: {
      int h = (int)(s_sleep / 60);
      int m = (int)(s_sleep % 60);
      snprintf(buf, len, "%dh%02d", h, m);
      break;
    }
    default:
      snprintf(buf, len, "--");
      break;
  }
}

// -- Health -----------------------------------------------------------------

static void refresh_health(void) {
#if defined(PBL_HEALTH)
  s_steps    = (int32_t)health_service_sum_today(HealthMetricStepCount);
  s_distance = (int32_t)health_service_sum_today(HealthMetricWalkedDistanceMeters);
  s_calories = (int32_t)health_service_sum_today(HealthMetricActiveKCalories)
             + (int32_t)health_service_sum_today(HealthMetricRestingKCalories);

  time_t midnight = time_start_of_today();
  time_t now      = time(NULL);
  HealthServiceAccessibilityMask mask =
    health_service_metric_accessible(HealthMetricSleepSeconds, midnight, now);
  if (mask & HealthServiceAccessibilityMaskAvailable) {
    s_sleep = (int32_t)(health_service_sum(HealthMetricSleepSeconds, midnight, now) / 60);
  }
#endif
}

static void health_event_handler(HealthEventType event, void *ctx) {
  if (event == HealthEventMovementUpdate ||
      event == HealthEventSleepUpdate    ||
      event == HealthEventMetricAlert) {
    refresh_health();
    if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
  }
}

// -- Drawing ----------------------------------------------------------------

static void draw_box(GContext *gc,
                     GRect rect,
                     int data_id,
                     GColor bg, GColor fg, GColor accent) {
  // Box background
  graphics_context_set_fill_color(gc, bg);
  graphics_fill_rect(gc, rect, 6, GCornersAll);

  // Accent strip at top
  GRect strip = GRect(rect.origin.x, rect.origin.y, rect.size.w, 4);
  graphics_context_set_fill_color(gc, accent);
  graphics_fill_rect(gc, strip, 6, GCornersTop);

  // Label
  char label_buf[12];
  snprintf(label_buf, sizeof(label_buf), "%s", data_label(data_id));
  GRect label_rect = GRect(rect.origin.x + 2,
                           rect.origin.y + 5,
                           rect.size.w - 4,
                           16);
  graphics_context_set_text_color(gc, fg);
  graphics_draw_text(gc, label_buf, s_font_label, label_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Value
  char val_buf[12];
  data_value_str(data_id, val_buf, sizeof(val_buf));
  GRect val_rect = GRect(rect.origin.x + 2,
                         rect.origin.y + 22,
                         rect.size.w - 4,
                         rect.size.h - 24);
  graphics_draw_text(gc, val_buf, s_font_value, val_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);
}

static void canvas_update_proc(Layer *layer, GContext *gc) {
  GRect bounds = layer_get_bounds(layer);
  int W = bounds.size.w;
  int H = bounds.size.h;

  // Colours
  GColor bg_col, fg_col, box_bg, box_fg, accent_a, accent_b;
  if (s_dark_mode) {
    bg_col  = GColorBlack;
    fg_col  = GColorWhite;
    box_bg  = GColorDarkGray;
    box_fg  = GColorWhite;
    accent_a = GColorCyan;
    accent_b = GColorMintGreen;
  } else {
    bg_col  = GColorWhite;
    fg_col  = GColorBlack;
    box_bg  = GColorLightGray;
    box_fg  = GColorBlack;
    accent_a = GColorBlue;
    accent_b = GColorGreen;
  }

  // Background
  graphics_context_set_fill_color(gc, bg_col);
  graphics_fill_rect(gc, bounds, 0, GCornerNone);

  // -- Time ------------------------------------------------------------------
  int top_h = H / 3;  // top third for time

  char time_buf[8];
  snprintf(time_buf, sizeof(time_buf), "%02d:%02d", s_hours, s_minutes);

  // LECO-42 glyphs are ~42px tall. Vertically centre within top_h.
  int font_h = 42;
  int time_y = (top_h - font_h) / 2;
  if (time_y < 0) time_y = 0;
  GRect time_rect = GRect(0, time_y, W, font_h + 4);
  graphics_context_set_text_color(gc, fg_col);
  graphics_draw_text(gc, time_buf, s_font_time, time_rect,
                     GTextOverflowModeTrailingEllipsis,
                     GTextAlignmentCenter, NULL);

  // Separator
  graphics_context_set_stroke_color(gc, s_dark_mode ? GColorDarkGray : GColorLightGray);
  graphics_context_set_stroke_width(gc, 1);
  graphics_draw_line(gc, GPoint(6, top_h), GPoint(W - 6, top_h));

  // -- Data boxes ------------------------------------------------------------
  int pad   = 4;
  int half_w = (W - pad * 3) / 2;
  int row_h  = (H - top_h - pad * 3) / 2;

  int x0 = pad;
  int x1 = pad * 2 + half_w;
  int y0 = top_h + pad;
  int y1 = top_h + pad * 2 + row_h;

  GColor accents[4] = { accent_a, accent_b, accent_b, accent_a };
  GRect rects[4] = {
    GRect(x0, y0, half_w, row_h),
    GRect(x1, y0, half_w, row_h),
    GRect(x0, y1, half_w, row_h),
    GRect(x1, y1, half_w, row_h),
  };

  for (int i = 0; i < 4; i++) {
    draw_box(gc, rects[i], s_box_cfg[i], box_bg, box_fg, accents[i]);
  }
}

// -- Tick ------------------------------------------------------------------

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
  s_hours   = tick_time->tm_hour;
  s_minutes = tick_time->tm_min;
  if (tick_time->tm_min % 5 == 0) refresh_health();
  if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

// -- AppMessage ------------------------------------------------------------

static void inbox_received(DictionaryIterator *iter, void *ctx) {
  Tuple *t;

  t = dict_find(iter, MESSAGE_KEY_DARK_MODE);
  if (t) {
    s_dark_mode = (bool)t->value->int32;
    persist_write_int(PERSIST_DARK_MODE, s_dark_mode ? 1 : 0);
  }

  const uint32_t box_keys[4] = {
    MESSAGE_KEY_BOX_A, MESSAGE_KEY_BOX_B, MESSAGE_KEY_BOX_C, MESSAGE_KEY_BOX_D
  };
  const int persist_keys[4] = {
    PERSIST_BOX_A, PERSIST_BOX_B, PERSIST_BOX_C, PERSIST_BOX_D
  };
  for (int i = 0; i < 4; i++) {
    t = dict_find(iter, box_keys[i]);
    if (t) {
      s_box_cfg[i] = (int)t->value->int32;
      persist_write_int(persist_keys[i], s_box_cfg[i]);
    }
  }

  if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

// -- Window ----------------------------------------------------------------

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_font_time  = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  s_font_label = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  s_font_value = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(root, s_canvas_layer);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  s_canvas_layer = NULL;
}

// -- App lifecycle ---------------------------------------------------------

static void init(void) {
  // Load persisted settings
  if (persist_exists(PERSIST_DARK_MODE))
    s_dark_mode = persist_read_int(PERSIST_DARK_MODE) != 0;
  const int persist_keys[4] = { PERSIST_BOX_A, PERSIST_BOX_B, PERSIST_BOX_C, PERSIST_BOX_D };
  for (int i = 0; i < 4; i++) {
    if (persist_exists(persist_keys[i]))
      s_box_cfg[i] = persist_read_int(persist_keys[i]);
  }

  refresh_health();

#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_event_handler, NULL);
#endif

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  app_message_open(256, 64);
  app_message_register_inbox_received(inbox_received);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_hours   = t->tm_hour;
  s_minutes = t->tm_min;

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}