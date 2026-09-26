#include <pebble.h>

#define BUFFER_LEN (40)
#define MINUTE_IN_MS (60 * 1000)

static Window* s_window;
static Layer* s_top_layer;
static Layer* s_bot_layer;
static time_t s_start;
static char s_buffer[BUFFER_LEN];
static GFont s_font_lg;

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_SELECT, 500, select_click_handler);

  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 250, up_click_handler);

  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 250, down_click_handler);
}

static void draw_text(GContext* ctx, const char* buf, GFont font, GRect bbox, int shift_up, GTextAlignment align) {
  GRect fixed = bbox;
  fixed.origin.y -= shift_up;
  graphics_draw_text(ctx, buf, font, fixed, GTextOverflowModeFill, align, NULL);
}

static void update_top_layer(Layer* layer, GContext* ctx) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "top");
  // Elapsed run time
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  time_t now = time(NULL);
  int elapsed_min = (now - s_start) / 60;
  int hours = elapsed_min / 60;
  int minutes = elapsed_min % 60;
  graphics_context_set_text_color(ctx, GColorWhite);
  snprintf(s_buffer, BUFFER_LEN, "%02d:%02d", hours, minutes);
  draw_text(ctx, s_buffer, s_font_lg, bounds, -5, GTextAlignmentCenter);
}

static void update_bot_layer(Layer* layer, GContext* ctx) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "bot");
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Time of day
  time_t now = time(NULL);
  struct tm* local = localtime(&now);
  if (clock_is_24h_style()) {
    strftime(s_buffer, BUFFER_LEN, "%H:%M", local);
  } else {
    strftime(s_buffer, BUFFER_LEN, "%l:%M", local);
  }
  graphics_context_set_text_color(ctx, GColorBlack);
  draw_text(ctx, s_buffer, s_font_lg, bounds, -5, GTextAlignmentCenter);
}

static void elapsed_minute_handler(void* unused) {
  app_timer_register(MINUTE_IN_MS, elapsed_minute_handler, NULL);
  layer_mark_dirty(s_top_layer);
}

static void tick_handler(struct tm* now, TimeUnits units_changed) {
  layer_mark_dirty(s_bot_layer);
}

static void window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  int x = bounds.origin.x;
  int y = bounds.origin.y;
  int hh = bounds.size.h / 2;
  int w = bounds.size.w;
  GRect top_bounds = GRect(x, y, w, hh);
  GRect bot_bounds = GRect(x, y + hh, w, hh);
  s_bot_layer = layer_create(bot_bounds);
  s_top_layer = layer_create(top_bounds);
  layer_set_update_proc(s_bot_layer, update_bot_layer);
  layer_set_update_proc(s_top_layer, update_top_layer);
  layer_add_child(window_layer, s_bot_layer);
  layer_add_child(window_layer, s_top_layer);

  app_timer_register(MINUTE_IN_MS, elapsed_minute_handler, NULL);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void window_unload(Window *window) {
  tick_timer_service_unsubscribe();
  layer_destroy(s_top_layer);
  s_top_layer = NULL;
  layer_destroy(s_bot_layer);
  s_bot_layer = NULL;
}

static void init(void) {
  s_start = time(NULL);
  s_window = window_create();
  s_font_lg = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CFONT_90));
  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}

static void deinit(void) {
  window_destroy(s_window);
  if (s_font_lg) { fonts_unload_custom_font(s_font_lg); }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
