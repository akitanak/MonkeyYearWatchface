#include <pebble.h>

#define THRESHOLD_BAD_WEATHER_LOWER 800
#define THRESHOLD_BAD_WEATHER_UPPER 804

static Window *s_main_window;
static TextLayer *s_time_layer;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

// Store incoming information
static char temperature_buffer[8];
static char condition_buffer[32];

enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITION,
  KEY_CONDITION_ID    
};

/*
 * Communicate Phone
 */
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  
  int condition_id = 800;
  
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
  Tuple *condition_tuple = dict_find(iterator, KEY_CONDITION);
  Tuple *condition_id_tuple = dict_find(iterator, KEY_CONDITION_ID);

  // If all data is available, use it
  if(temp_tuple && condition_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
    snprintf(condition_buffer, sizeof(condition_buffer), "%s", condition_tuple->value->cstring);
  }
  
  if(condition_id_tuple) {
    condition_id = condition_id_tuple->value->int32;
    if(condition_id < THRESHOLD_BAD_WEATHER_LOWER || condition_id > THRESHOLD_BAD_WEATHER_UPPER) {
      APP_LOG(APP_LOG_LEVEL_INFO, "face change to BAD weather.");
      s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MONKEY_WATCHFACE_SAD_PNG);
      bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    } else {
      APP_LOG(APP_LOG_LEVEL_INFO, "face change to FINE weather.");
      s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MONKEY_WATCHFACE_PNG);
      bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    }
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}


/*
 * update time
 */
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

/*
 * load Main window
 */
static void main_window_load(Window *window) {

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create GBitmap
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_MONKEY_WATCHFACE_PNG);

  // Create BitmapLayer to display the GBitmap
  s_background_layer = bitmap_layer_create(bounds);

  // Set the bitmap onto the layer and add to the window
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, 45, bounds.size.w, 42));

  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_background_color(s_time_layer, GColorClear);

  // update current time
  update_time();

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }
  
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);

  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
}

/*
 * Initialize
 */
static void init() {

  s_main_window = window_create();

  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(s_main_window, true);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
