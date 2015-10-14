#include <pebble.h>
#include "train.h"

#define TRAIN_IMAGES_COUNT 12  

#define KEY_TRAIN_TITLE 0
#define KEY_TRAIN_TIME 1
#define KEY_TRAIN_COUNT 2
#define KEY_EXIT_TIME 3
#define KEY_SHEDULE_SENT 4
#define KEY_COMMAND 5
#define KEY_TRACK_TITLE 6
#define KEY_STATION_DISTANCE 7
#define KEY_TRAIN_NUMBER 8

typedef struct {
  char title[150];
  time_t time;
  time_t exit_time;
  uint16_t station_dest;
} Train;

static Train *s_shedule_array;

static BitmapLayer *s_train_layer;
static GBitmap *s_train_bitmap[TRAIN_IMAGES_COUNT];

static TextLayer *s_track_layer;

static int16_t s_train_index  = 0;
static uint16_t s_trains_count = 0;
static uint8_t s_shedule_received = 0;

static void send_command(char * command) {
  DictionaryIterator* dictionaryIterator = NULL;
  app_message_outbox_begin (&dictionaryIterator);
  dict_write_cstring (dictionaryIterator, KEY_COMMAND, command);
  dict_write_end (dictionaryIterator);
  app_message_outbox_send ();
}

void update_train() {
  
  time_t current_time = time(NULL);
  
  if (s_shedule_received == 1){
    s_train_index = -1;
    for (uint16_t i = 0; i < s_trains_count; i++){
      if (current_time < s_shedule_array[i].exit_time){
        APP_LOG(APP_LOG_LEVEL_INFO, "Current train is %s", s_shedule_array[i].title);
        APP_LOG(APP_LOG_LEVEL_INFO, "train time %d", (int) s_shedule_array[i].time);
        APP_LOG(APP_LOG_LEVEL_INFO, "station dest %d", (int) s_shedule_array[i].station_dest);
        APP_LOG(APP_LOG_LEVEL_INFO, "current  time %d", (int) current_time);
  
        s_train_index = i;
        uint32_t time_difference = s_shedule_array[i].exit_time - current_time;

        APP_LOG(APP_LOG_LEVEL_INFO, "time_difference %d", (int) time_difference);

        if (time_difference > 60 * 9) {
          bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[9]);
        } else {
          uint8_t index = time_difference / 60;
          APP_LOG(APP_LOG_LEVEL_INFO, " get image %d", index);
          bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[index]);
        }
        
        static char time_str[] = "00:00";
        
        struct tm *tick_time = localtime(&s_shedule_array[i].time);
        strftime(time_str, sizeof("00:00"), "%H:%M", tick_time);

        static char title_str[160];
        snprintf(title_str, sizeof(title_str), "%s : %s", s_shedule_array[i].title, time_str);

        APP_LOG(APP_LOG_LEVEL_INFO, " get image %s", title_str);
        
        text_layer_set_text(s_track_layer, title_str);

        break;
      }
    }
    if (s_train_index < 0) {
        bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[11]);
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
  s_train_bitmap[11] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_THINK);
  
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

  s_track_layer = text_layer_create(GRect(0, 108, 144, 168));
  text_layer_set_background_color(s_track_layer, GColorBlack);
  text_layer_set_text_color(s_track_layer, GColorClear);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_track_layer));
}

void train_unload(){
    for(int i = 0; i < TRAIN_IMAGES_COUNT; i++) {
      gbitmap_destroy(s_train_bitmap[i]);
    }

    bitmap_layer_destroy(s_train_layer);
    free(s_shedule_array);
    text_layer_destroy(s_track_layer);

}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  APP_LOG(APP_LOG_LEVEL_INFO, "read");
  Tuple *t = dict_read_first(iterator);
  APP_LOG(APP_LOG_LEVEL_INFO, "readed");

  Train train;
  bool is_train = false;
  uint8_t train_number = 0;

  // For all items
  while(t != NULL) {
    // Which key was received?
     APP_LOG(APP_LOG_LEVEL_INFO, "Key %d",(int) t->key);
    switch(t->key) {
      case KEY_TRAIN_COUNT:
        APP_LOG(APP_LOG_LEVEL_INFO, "Trains count %d", t->value->uint16);

        s_train_index = -1;
        s_shedule_received = 0;
        bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[10]);
        s_trains_count = t->value->uint16;

        free(s_shedule_array);
        s_shedule_array = (Train*)malloc(s_trains_count * sizeof(Train));
      
        send_command("send_next_train");
        break;
      case KEY_TRAIN_TITLE:
        APP_LOG(APP_LOG_LEVEL_INFO, "tit %s", t->value->cstring);

        is_train = true;
        snprintf(train.title, sizeof(train.title), "%s", t->value->cstring);

        break;
      case KEY_TRAIN_TIME:
        APP_LOG(APP_LOG_LEVEL_INFO, "tim %d", (int) t->value->uint32);

        train.time = (time_t) t->value->uint32;

        break;
      case KEY_EXIT_TIME:
        APP_LOG(APP_LOG_LEVEL_INFO, "exit %d", (int) t->value->uint32);

        train.exit_time = (time_t) t->value->uint32;

        break;
      case KEY_STATION_DISTANCE:
        APP_LOG(APP_LOG_LEVEL_INFO, "dest %d", (int) t->value->uint16);

        train.station_dest = t->value->uint16;

        break;
      case KEY_TRAIN_NUMBER:
        APP_LOG(APP_LOG_LEVEL_INFO, "dest %d", (int) t->value->uint16);

        train_number = t->value->uint8;

        break;
      case KEY_SHEDULE_SENT:
        APP_LOG(APP_LOG_LEVEL_INFO, "sent %d", (int) t->value->uint8);

        s_shedule_received = (uint8_t)t->value->uint8;
        update_train();

        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  if (is_train) {
    s_shedule_array[train_number] = train;
    send_command("send_next_train");
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