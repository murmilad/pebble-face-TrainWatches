#include <pebble.h>
#include "train.h"

#define TRAIN_IMAGES_COUNT 11  

#define KEY_TRAIN_TITLE 0
#define KEY_TRAIN_TIME 1
#define KEY_TRAIN_COUNT 2
#define KEY_EXIT_TIME 3
#define KEY_SHEDULE_SENT 4
#define KEY_COMMAND 5
#define KEY_TRACK_TITLE 6
#define KEY_STATION_DISTANCE 7
#define KEY_TRAIN_NUMBER 8
#define KEY_STATION_COUNT 9
#define KEY_STATION_TITLE 10
#define KEY_STATION_NUMBER 11
#define KEY_TRAIN_STATION_FROM 12
#define KEY_TRAIN_STATION_TO 13

typedef struct {
  uint8_t station_from;
  uint8_t station_to;
  time_t time;
  time_t exit_time;
  uint16_t station_dest;
} Train;

typedef struct {
  char title[110];
  uint8_t number;
} Station;

static Train *s_shedule_array;
static Station *s_stations_array;

static BitmapLayer *s_train_layer;
static BitmapLayer *s_frame_layer;
static GBitmap *s_train_bitmap[TRAIN_IMAGES_COUNT];

static TextLayer *s_track_layer;
static TextLayer *s_timer_layer;

static int16_t s_train_index  = 0;
static uint16_t s_trains_count = 0;
static bool s_shedule_received = false;
static GFont s_timer_font;
static GFont s_track_font;
static time_t s_train_time = 0;

static void send_command(char * command) {
  DictionaryIterator* dictionaryIterator = NULL;
  app_message_outbox_begin (&dictionaryIterator);
  dict_write_cstring (dictionaryIterator, KEY_COMMAND, command);
  dict_write_end (dictionaryIterator);
  app_message_outbox_send ();
}

void update_train_timer() {
  if (s_train_time != 0) {
    time_t timer = s_train_time - time(NULL);
    struct tm *tick_time = localtime(&timer);
  
    static char time_str[] = "00:00";
    strftime(time_str, sizeof("00:00"), "%M:%S", tick_time);
  
    text_layer_set_text(s_timer_layer, time_str);
  } else {
    text_layer_set_text(s_timer_layer, "");
  }
}


void update_train_second() {
  update_train_timer();
}

void update_train_minute() {
  
  time_t current_time = time(NULL);
  
  if (s_shedule_received){
    s_train_index = -1;
    for (uint16_t i = 0; i < s_trains_count; i++){
      if (current_time < s_shedule_array[i].time){
  
        s_train_index = i;
        int32_t time_difference = s_shedule_array[i].exit_time - current_time;

          s_train_time = s_shedule_array[i].time;

        if (time_difference > 60 * 9) {
          s_train_time = 0;
          update_train_timer();
          bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[9]);
        } else if (time_difference < 0){
          s_train_index = i;

          s_train_time = s_shedule_array[i].time;

          bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[9]);
          
          i++;
        } else {

          uint8_t index = time_difference / 60;
          s_train_time = 0;
          update_train_timer();
          bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[index]);
        }

        if (i < s_trains_count) {

          static char time_str[] = "00:00";

          struct tm *tick_time = localtime(&s_shedule_array[i].time);
          strftime(time_str, sizeof("00:00"), "%H:%M", tick_time);

          static char title_str[350];

          if (s_train_index == i) {
            snprintf(title_str, sizeof(title_str), "%s %s - %s", time_str, s_stations_array[s_shedule_array[i].station_from].title, s_stations_array[s_shedule_array[i].station_to].title);
          } else {
            snprintf(title_str, sizeof(title_str), "Следующий: %s %s - %s", time_str, s_stations_array[s_shedule_array[i].station_from].title, s_stations_array[s_shedule_array[i].station_to].title);
          }
  
          text_layer_set_text(s_track_layer, title_str);
        } else {
          text_layer_set_text(s_track_layer, "Следующих поездов больше нет");
        }

        break;
      }
    }
    if (s_train_index < 0) {
        text_layer_set_text(s_track_layer, "На сегодня поездов больше нет");
    }
  
    if(current_time % (60 * 60 * 24) == 0) {
      send_command("get_shedule");
    }
  } else {
    bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[10]);
  }
}

void train_load(Window *window) {
    // Create GBitmap, then set to created BitmapLayer
  s_train_bitmap[10] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_THINK_LOAD);
  
  s_train_bitmap[9] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN);
  s_train_bitmap[8] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_1);
  s_train_bitmap[7] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_2);
  s_train_bitmap[6] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_3);
  s_train_bitmap[5] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_4);
  s_train_bitmap[4] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_5);
  s_train_bitmap[3] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_6);
  s_train_bitmap[2] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_7);
  s_train_bitmap[1] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_8);
  s_train_bitmap[0] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_9);


  s_train_layer = bitmap_layer_create(GRect(83, 25, 61, 61));
  bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[10]);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_train_layer));

  s_frame_layer = bitmap_layer_create(GRect(0, 86, 144, 82));
  bitmap_layer_set_bitmap(s_frame_layer, gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_FRAME));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_frame_layer));


  s_track_layer = text_layer_create(GRect(10, 103, 130, 62));
  text_layer_set_background_color(s_track_layer, GColorBlack);
  text_layer_set_text_color(s_track_layer, GColorClear);
  s_track_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GEORGIA_PEBBLE14));
  text_layer_set_font(s_track_layer, s_track_font);

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_track_layer));
  
  s_timer_layer = text_layer_create(GRect(93, 25, 61, 20));
  text_layer_set_background_color(s_timer_layer, GColorClear );
  text_layer_set_text_color(s_timer_layer, GColorClear);
  s_timer_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GEORGIA17));
  text_layer_set_font(s_timer_layer, s_timer_font);

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_timer_layer));

}

void train_unload(){
    for(int i = 0; i < TRAIN_IMAGES_COUNT; i++) {
      gbitmap_destroy(s_train_bitmap[i]);
    }

    bitmap_layer_destroy(s_train_layer);
    bitmap_layer_destroy(s_frame_layer);
    free(s_shedule_array);
    text_layer_destroy(s_track_layer);
    text_layer_destroy(s_timer_layer);

}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);

  Train train;
  bool is_train = false;
  uint8_t train_number = 0;
  
  Station station;
  bool is_station = false;

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
      case KEY_STATION_COUNT:
        free(s_stations_array);
        s_stations_array = (Station*)malloc(t->value->uint8 * sizeof(Station));
        send_command("send_next_station");
        break;
      case KEY_STATION_TITLE:
        is_station = true;
        snprintf(station.title, sizeof(station.title), "%s", t->value->cstring);
        break;
      case KEY_STATION_NUMBER:
        station.number = t->value->uint8;
        break;
      case KEY_TRAIN_COUNT:

        s_train_time = 0;
        update_train_timer();
        s_train_index = -1;
        s_shedule_received = false;
        bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[10]);
        s_trains_count = t->value->uint16;

        free(s_shedule_array);
        s_shedule_array = (Train*)malloc(s_trains_count * sizeof(Train));
      
        send_command("send_next_train");
        break;
      case KEY_TRAIN_STATION_FROM:
        train.station_from = t->value->uint8;

        break;
      case KEY_TRAIN_STATION_TO:
        train.station_to = t->value->uint8;

        break;
      case KEY_TRAIN_TIME:

        is_train = true;

        train.time = (time_t) t->value->uint32;

        break;
      case KEY_EXIT_TIME:

        train.exit_time = (time_t) t->value->uint32;

        break;
      case KEY_STATION_DISTANCE:

        train.station_dest = t->value->uint16;

        break;
      case KEY_TRAIN_NUMBER:

        train_number = t->value->uint8;

        break;
      case KEY_SHEDULE_SENT:

        s_shedule_received = true;
        update_train_minute();

        break;
      default:
        break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  if (is_train) {
    s_shedule_array[train_number] = train;
    send_command("send_next_train");
  }
  if (is_station) {
    s_stations_array[station.number] = station;
    send_command("send_next_station");
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

void train_init(){
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}