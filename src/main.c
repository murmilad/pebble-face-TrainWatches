#include <pebble.h>
#include "train.h"


static Window *s_main_window;
static TextLayer *s_time_layer;
static GFont s_time_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
      if (units_changed & MINUTE_UNIT) {
        update_time();
        update_train_minute();
      }
      if (units_changed & SECOND_UNIT) {
        update_train_second();
      }
}

static void main_window_load(Window *window) {
  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GIRL);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(80, 0, 61, 25));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorClear);
  text_layer_set_text(s_time_layer, "00:00");

  // Improve the layout to be more like a watchface
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GEORGIA21));

  // Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);

  // Load train data
  train_load(window);
}

static void main_window_unload(Window *window) {
    // Load train data
    train_unload();

    // Destroy TextLayer
    text_layer_destroy(s_time_layer);

    // Unload GFont
    fonts_unload_custom_font(s_time_font);

    // Destroy BitmapLayer
    bitmap_layer_destroy(s_background_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT | SECOND_UNIT, tick_handler);

  // Init train
  train_init();
  
  // Make sure the time is displayed from the start
  update_time();

}

static void deinit() {
    // Destroy GBitmap
    gbitmap_destroy(s_background_bitmap);

    // Destroy Window
    window_destroy(s_main_window);
}


int main(void) {
  init();
  app_event_loop();
  deinit();
}
